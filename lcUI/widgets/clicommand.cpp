#include "clicommand.h"
#include "ui_clicommand.h"
#include <memory>
#include <iterator>
#include <unordered_map>
#include <QTextDocument>
#include <QPainter>
#include <QScrollBar>
#include <QMessageBox>
#include <QRegularExpression>

using namespace lc::ui::widgets;

CliCommand::CliCommand(QWidget* parent) :
    QDockWidget(parent),
    ui(new Ui::CliCommand),
    _returnText(false),
    _commandActive(false),
    _historySize(10),
    _historyIndex(-1) {
    ui->setupUi(this);

    connect(ui->command, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));

    _commands = std::make_shared<QStringListModel>();
    _completer = std::make_shared<QCompleter>();

    _completer->setCaseSensitivity(Qt::CaseInsensitive);
    _completer->setCompletionMode(QCompleter::InlineCompletion);
    _completer->setModel(_commands.get());

    ui->command->setCompleter(_completer.get());


    WidgetTitleBar* titleBar = new WidgetTitleBar( "Cli Command", this,
            WidgetTitleBar::TitleBarOptions::HorizontalOnHidden);
    this->setTitleBarWidget(titleBar);
}

CliCommand::~CliCommand() {
    delete ui;
}

bool CliCommand::addCommand(const char* name, kaguya::LuaRef cb) {
    if(_commands->stringList().indexOf(name) == -1) {
        auto newList = _commands->stringList();
        newList << QString(name);
        _commands->setStringList(newList);
        _commands_cb[QString(name)] = cb;
        _commands_enabled[QString(name)] = true;
        return true;
    }
    else {
        return false;
    }
}

void CliCommand::write(std::string message) {
    ui->history->setHtml(ui->history->toHtml() + QString(message.c_str()));
    ui->history->verticalScrollBar()->setValue(ui->history->verticalScrollBar()->maximum());
}

void CliCommand::onReturnPressed() {
    auto text = ui->command->text();
    bool isNumber;
    QStringList varFind;
    QRegularExpression re("^([a-zA-Z]{1,10}+)=([0-9]{1,50}.[0-9]{1,50})|([0-9]{1,50})$");
    QRegularExpressionMatch match = re.match(text, 0, QRegularExpression::PartialPreferCompleteMatch);
    bool hasMatch = match.hasMatch();

    if(_returnText) {
        emit textEntered(text);
    }
    else if(text != "") {
        _history.push_front(text);

        if (_history.size() > _historySize) {
            _history.pop_back();
        }

        auto number = text.toDouble(&isNumber);
        if (isNumber) {
            enterNumber(number);
        }
        else if (text.indexOf(";") != -1 || text.indexOf(",") != -1) {
            enterCoordinate(text);
        }
        else if(hasMatch) {
            varFind = text.split("=");

            if(checkParam(varFind[0])) {
                /*write(QString("Value of %1 = %2").arg(varFind[0]).arg(varFind[1].toFloat()));
                lc::storage::Settings::setVal(varFind[0].toStdString(),varFind[1].toFloat());*/
            }
            else {
                write("No such variable.");
            }
        }
        else {
            /* Check for command status and nested command like 'zoom or 'pan which can run in between some other active command.*/
            if ((_commandActive) && (text.indexOf("'") != 0)) {
                emit textEntered(text);
            }
            else {
                enterCommand(text);
            }
        }
    }

    _historyIndex = -1;
    ui->command->clear();
}

void CliCommand::keyPressEvent(QKeyEvent *event) {
    onKeyPressed(event);
}

void CliCommand::enterCommand(const QString& command) {
    auto completion = _completer->currentCompletion();

    if(command.compare(completion, Qt::CaseInsensitive) == 0) {
        write("Command: " + completion.toStdString());
        emit commandEntered(completion);
    }
    else {
        if(checkParam(command)) {
            //write(QString("Value of %1=%2").arg(command).arg(lc::storage::Settings::val(command.toStdString())));
        }
        else
        {
            write("Command " + command.toStdString() + " not found");
        }
    }
}

bool CliCommand::checkParam(const QString& command) {
    return false;//lc::storage::Settings::exists(command.toStdString());
}

void CliCommand::enterCoordinate(QString coordinate) {
    lc::geo::Coordinate point;
    QStringList numbers;
    bool isRelative = false;

    if(coordinate.indexOf("@") != -1) {
        isRelative = true;
        coordinate.remove("@");
    }

    if(coordinate.indexOf(";") != -1) {
        numbers = coordinate.split(";");
    }
    else {
        numbers = coordinate.split(",");
    }


    if(numbers.size() > 2) {
        point = lc::geo::Coordinate(numbers[0].toFloat(), numbers[1].toFloat(), numbers[2].toFloat());
    }
    else {
        point = lc::geo::Coordinate(numbers[0].toFloat(), numbers[1].toFloat());
    }

    auto message = QString("Coordinate: x=%1; y=%2; z=%3").arg(point.x()).arg(point.y()).arg(point.z());
    write(message.toStdString());

    if(isRelative) {
        emit relativeCoordinateEntered(point);
    }
    else {
        emit coordinateEntered(point);
    }
}

void CliCommand::enterNumber(double number) {
    write((QString("Number: %1").arg(number)).toStdString().c_str());
    emit numberEntered(number);
}

void CliCommand::onKeyPressed(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Up:

        if(_historyIndex + 1 < _history.size()) {
            _historyIndex++;
            ui->command->setText(_history[_historyIndex]);
        }
        break;

    case Qt::Key_Down:
        if(_historyIndex > 0) {
            _historyIndex--;
            ui->command->setText(_history[_historyIndex]);
        }
        else {
            _historyIndex = -1;
            ui->command->clear();
        }
        break;

    case Qt::Key_Escape:
        emit finishOperation();
        break;

    default:
        ui->command->event(event);
        break;
    }
}

void CliCommand::setText(const QString& text) {
    ui->command->setText(text);
}

void CliCommand::returnText(bool returnText) {
    _returnText = returnText;
}

void CliCommand::commandActive(bool commandActive) {
    _commandActive = commandActive;
}

void CliCommand::closeEvent(QCloseEvent* event)
{
    this->widget()->hide();
    event->ignore();
}

void CliCommand::runCommand(const char* command)
{
    if (_commands_cb.find(command) == _commands_cb.end()) {
        return;
    }

    if (!_commands_enabled[command]) {
        write(std::string(command) + " command has been disabled.");
        return;
    }
    _commands_entered.push_back(command);
    kaguya::LuaRef& cb = _commands_cb[command];
    cb();
}

void CliCommand::enableCommand(const char* command, bool enable) {
    if (_commands_enabled.find(command) == _commands_enabled.end()) {
        return;
    }

    _commands_enabled[command] = enable;
}

bool CliCommand::isCommandEnabled(const char* command) const {
    if (_commands_enabled.find(QString(command)) == _commands_enabled.end()) {
        return false;
    }

    return _commands_enabled[QString(command)];
}

std::vector<std::string> CliCommand::availableCommands() const {
    std::vector<std::string> allcommands;

    for (auto command : _commands_enabled.keys()) {
        allcommands.push_back(command.toStdString());
    }

    return allcommands;
}

std::vector<std::string> CliCommand::commandsHistory() const {
    return _commands_entered;
}

void CliCommand::clear() {
    ui->history->setHtml(QString(""));
}
