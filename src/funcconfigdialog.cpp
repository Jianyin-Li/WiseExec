#include "funcconfigdialog.h"
#include "../ui/ui_funcconfigdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QInputDialog>

FuncConfigDialog::FuncConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FuncConfigDialog)
    , newFunc(nullptr)
    , existingItem(nullptr)
{
    ui->setupUi(this);
    
    // 连接信号槽，使用Qt6新语法
    connect(ui->selectIconBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectIconClicked);
    connect(ui->addCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onAddCmdClicked);
    connect(ui->delCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onDelCmdClicked);
    connect(ui->selectExeBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectExeClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &FuncConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &FuncConfigDialog::onCancelClicked);
    
    // 设置图标路径输入框为只读
    ui->iconPathEdit->setReadOnly(true);
}

FuncConfigDialog::FuncConfigDialog(FuncItem *existingItem, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FuncConfigDialog)
    , newFunc(nullptr)
    , existingItem(existingItem)
{
    ui->setupUi(this);
    
    // 连接信号槽
    connect(ui->selectIconBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectIconClicked);
    connect(ui->addCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onAddCmdClicked);
    connect(ui->delCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onDelCmdClicked);
    connect(ui->selectExeBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectExeClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &FuncConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &FuncConfigDialog::onCancelClicked);
    
    // 设置图标路径输入框为只读
    ui->iconPathEdit->setReadOnly(true);
    
    // 从现有项初始化对话框
    if (existingItem) {
        initFromItem(existingItem);
    }
}

FuncConfigDialog::~FuncConfigDialog()
{
    delete ui;
}

void FuncConfigDialog::initFromItem(FuncItem *item)
{
    if (!item) return;
    
    ui->funcNameEdit->setText(item->getName());
    ui->iconPathEdit->setText(item->getIconPath());
    
    // 清空现有命令列表并添加
    ui->cmdListWidget->clear();
    for (const QString &cmd : item->getCmds()) {
        ui->cmdListWidget->addItem(cmd);
    }
}

void FuncConfigDialog::onSelectIconClicked()
{
    // 跨平台文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择功能图标"),
        QDir::currentPath(),
        tr("图片文件 (*.png *.jpg *.ico *.bmp *.svg)"));
    
    if (!fileName.isEmpty()) {
        ui->iconPathEdit->setText(fileName);
    }
}

void FuncConfigDialog::onAddCmdClicked()
{
    // 弹出输入对话框让用户输入命令
    bool ok;
    QString command = QInputDialog::getText(this, 
        tr("输入命令"),
        tr("请输入要执行的命令（例如：notepad.exe、calc.exe、cmd /k \"echo Hello\"）:"),
        QLineEdit::Normal,
        "",
        &ok);
    
    if (ok && !command.trimmed().isEmpty()) {
        // 检查是否已存在
        bool exists = false;
        for (int i = 0; i < ui->cmdListWidget->count(); ++i) {
            if (ui->cmdListWidget->item(i)->text() == command.trimmed()) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            ui->cmdListWidget->addItem(command.trimmed());
        } else {
            QMessageBox::information(this, tr("提示"), tr("该命令已存在"));
        }
    }
}

void FuncConfigDialog::onSelectExeClicked()
{
    // 跨平台文件选择对话框，按系统自动适配过滤规则
    QString filter;
#ifdef Q_OS_WIN
    filter = tr("可执行文件 (*.exe *.bat *.cmd);;所有文件 (*.*)");
#else
    filter = tr("可执行文件 (*.sh *.bin);;所有文件 (*)");
#endif
    
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择可执行文件"),
        QDir::currentPath(),
        filter);
    
    if (!fileName.isEmpty()) {
        // 将文件路径添加到命令列表
        bool exists = false;
        for (int i = 0; i < ui->cmdListWidget->count(); ++i) {
            if (ui->cmdListWidget->item(i)->text() == fileName) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            ui->cmdListWidget->addItem(fileName);
        } else {
            QMessageBox::information(this, tr("提示"), tr("该文件已存在"));
        }
    }
}

void FuncConfigDialog::onDelCmdClicked()
{
    QListWidgetItem *currentItem = ui->cmdListWidget->currentItem();
    if (currentItem) {
        delete currentItem;
    }
}

void FuncConfigDialog::onConfirmClicked()
{
    QString funcName = ui->funcNameEdit->text().trimmed();
    QString iconPath = ui->iconPathEdit->text().trimmed();
    
    // 校验功能名非空
    if (funcName.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("功能名不能为空"));
        return;
    }
    
    // 如果指定了图标路径，检查文件是否存在
    if (!iconPath.isEmpty() && !QFile::exists(iconPath)) {
        QMessageBox::warning(this, tr("提示"), tr("图标文件不存在"));
        return;
    }
    
    // 校验命令列表非空
    if (ui->cmdListWidget->count() == 0) {
        QMessageBox::warning(this, tr("提示"), tr("命令列表不能为空"));
        return;
    }
    
    // 收集命令列表
    QStringList cmds;
    for (int i = 0; i < ui->cmdListWidget->count(); ++i) {
        cmds.append(ui->cmdListWidget->item(i)->text());
    }
    
    // 创建新的功能项（图标路径可以为空，将使用默认图标）
    newFunc = new FuncItem(funcName, iconPath, cmds, this);
    accept();
}

void FuncConfigDialog::onCancelClicked()
{
    reject();
}

FuncItem* FuncConfigDialog::getNewFunc() const
{
    return newFunc;
}