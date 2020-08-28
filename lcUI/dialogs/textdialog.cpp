#include "textdialog.h"
#include "ui_textdialog.h"

#include <cad/builders/text.h>

using namespace lc::ui::dialog;

TextDialog::TextDialog(lc::ui::MainWindow* mainWindowIn, QWidget* parent)
    : QDialog(parent),
    _mainWindow(mainWindowIn),
    ui(new Ui::TextDialog)
{
    ui->setupUi(this);

    const std::vector<std::pair<QString, QString>> symbolData = {
        {"Diameter(⌀)", "⌀"},
        {"Degree(°)", "°"},
        {"Plus / Minus(±)", "±"},
        {"At(@)", "@"},
        {"Hash(#)", "#"},
        {"Dollar($)", "$"},
        {"Copyright(©)", "©"},
        {"Registered(®)", "®"},
        {"Paragraph(§)", "§"},
        {"Pi(π)", "π"},
        {"Pound(£)", "£"},
        {"Yen(¥)", "¥"},
        {"Times(×)", "×"},
        {"Division(÷)", "÷"}
    };

    textEdit = qobject_cast<QTextEdit*>(ui->verticalLayout->itemAt(0)->widget());
    fontComboBox = qobject_cast<QComboBox*>(ui->horizontalLayout_1->itemAt(1)->widget());
    drawingDirectionComboBox = qobject_cast<QComboBox*>(ui->horizontalLayout->itemAt(1)->widget());
    halignComboBox = qobject_cast<QComboBox*>(ui->horizontalLayout_2->itemAt(1)->widget());
    valignComboBox = qobject_cast<QComboBox*>(ui->horizontalLayout_3->itemAt(1)->widget());
    heightSpinBox = qobject_cast<QDoubleSpinBox*>(ui->horizontalLayout_4->itemAt(1)->widget());
    angleSpinBox = qobject_cast<QDoubleSpinBox*>(ui->horizontalLayout_5->itemAt(1)->widget());

    drawingDirectionComboBox->addItem("None", lc::TextConst::DrawingDirection::None);
    drawingDirectionComboBox->addItem("Backward", lc::TextConst::DrawingDirection::Backward);
    drawingDirectionComboBox->addItem("Upside Down", lc::TextConst::DrawingDirection::UpsideDown);
    drawingDirectionComboBox->addItem("Backward | Upside Down", lc::TextConst::DrawingDirection::Backward | lc::TextConst::DrawingDirection::UpsideDown);

    halignComboBox->addItem("HALeft", lc::TextConst::HAlign::HALeft);
    halignComboBox->addItem("HACenter", lc::TextConst::HAlign::HACenter);
    halignComboBox->addItem("HARight", lc::TextConst::HAlign::HARight);
    halignComboBox->addItem("HAAligned", lc::TextConst::HAlign::HAAligned);
    halignComboBox->addItem("HAMiddle", lc::TextConst::HAlign::HAMiddle);
    halignComboBox->addItem("HAFit", lc::TextConst::HAlign::HAFit);

    valignComboBox->addItem("VABaseline", lc::TextConst::VAlign::VABaseline);
    valignComboBox->addItem("VABottom", lc::TextConst::VAlign::VABottom);
    valignComboBox->addItem("VAMiddle", lc::TextConst::VAlign::VAMiddle);
    valignComboBox->addItem("VATop", lc::TextConst::VAlign::VATop);

    std::vector<std::string> fontList = _mainWindow->cadMdiChild()->viewer()->docCanvas()->getFontList();
    for (const std::string& font : fontList) {
        fontComboBox->addItem(QString(font.c_str()), QString(font.c_str()));
    }

    heightSpinBox->setMaximum(100000);
    heightSpinBox->setValue(100);
    angleSpinBox->setMaximum(360);
    angleSpinBox->setWrapping(true);
    angleSpinBox->setValue(0);

    QComboBox* symbolComboBox = qobject_cast<QComboBox*>(ui->verticalLayout_8->itemAt(0)->widget());
    for (const std::pair<QString, QString>& symbol : symbolData) {
        symbolComboBox->addItem(symbol.first, symbol.second);
    }

    QPushButton* okButton = qobject_cast<QPushButton*>(ui->horizontalLayout_6->itemAt(0)->widget());
    QPushButton* cancelButton = qobject_cast<QPushButton*>(ui->horizontalLayout_6->itemAt(1)->widget());
    QPushButton* insertSymbolButton = qobject_cast<QPushButton*>(ui->verticalLayout_8->itemAt(1)->widget());
    connect(okButton, &QPushButton::released, this, &TextDialog::okButtonClicked);
    connect(cancelButton, &QPushButton::released, this, &TextDialog::cancelButtonClicked);
    connect(insertSymbolButton, &QPushButton::released, this, &TextDialog::insertSymbolClicked);
}

TextDialog::~TextDialog()
{
    delete ui;
}

void TextDialog::okButtonClicked() {
    kaguya::State state(_mainWindow->luaInterface()->luaState());
    _mainWindow->runOperation(state["TextOperations"]);
    state.dostring("updateTextOp = function(textEntity) mainWindow:currentOperation():copyEntity(textEntity) end");
    lc::builder::TextBuilder textBuilder;
    textBuilder.setLayer(_mainWindow->cadMdiChild()->activeLayer());
    textBuilder.setMetaInfo(_mainWindow->cadMdiChild()->metaInfoManager()->metaInfo());
    textBuilder.setBlock(_mainWindow->cadMdiChild()->activeViewport());

    textBuilder.setTextValue(textEdit->toPlainText().toStdString());
    textBuilder.setTextFont(fontComboBox->itemData(fontComboBox->currentIndex()).toString().toStdString());
    textBuilder.setHeight(heightSpinBox->value());
    textBuilder.setAngle(angleSpinBox->value() * 3.1416/180);
    textBuilder.setDrawingDirection((lc::TextConst::DrawingDirection)drawingDirectionComboBox->itemData(drawingDirectionComboBox->currentIndex()).toInt());
    textBuilder.setHorizontalAlign((lc::TextConst::HAlign)halignComboBox->itemData(halignComboBox->currentIndex()).toInt());
    textBuilder.setVerticalAlign((lc::TextConst::VAlign)valignComboBox->itemData(valignComboBox->currentIndex()).toInt());

    state["updateTextOp"](textBuilder.build());

    this->close();
}

void TextDialog::cancelButtonClicked() {
    this->close();
}

void TextDialog::insertSymbolClicked() {
    QComboBox* symbolComboBox = qobject_cast<QComboBox*>(ui->verticalLayout_8->itemAt(0)->widget());

    if (symbolComboBox != nullptr) {
        textEdit->insertPlainText(symbolComboBox->itemData(symbolComboBox->currentIndex()).toString());
    }
}
