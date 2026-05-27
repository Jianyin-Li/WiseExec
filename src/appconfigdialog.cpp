#include "appconfigdialog.h"
#include "../ui/ui_appconfigdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QFile>
#include <QDir>

AppConfigDialog::AppConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppConfigDialog)
    , newApp(nullptr)
    , existingItem(nullptr)
{
    ui->setupUi(this);
    
    // 连接信号槽，使用Qt6新语法
    connect(ui->selectIconBtn, &QPushButton::clicked, this, &AppConfigDialog::onSelectIconClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &AppConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &AppConfigDialog::onCancelClicked);
    
    // 设置图标路径输入框为只读
    ui->iconPathEdit->setReadOnly(true);
}

AppConfigDialog::AppConfigDialog(AppItem *existingItem, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AppConfigDialog)
    , newApp(nullptr)
    , existingItem(existingItem)
{
    ui->setupUi(this);
    
    // 连接信号槽
    connect(ui->selectIconBtn, &QPushButton::clicked, this, &AppConfigDialog::onSelectIconClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &AppConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &AppConfigDialog::onCancelClicked);
    
    // 设置图标路径输入框为只读
    ui->iconPathEdit->setReadOnly(true);
    
    // 从现有项初始化对话框
    if (existingItem) {
        initFromItem(existingItem);
    }
}

AppConfigDialog::~AppConfigDialog()
{
    delete ui;
}

void AppConfigDialog::initFromItem(AppItem *item)
{
    if (!item) return;
    
    ui->appNameEdit->setText(item->getName());
    ui->iconPathEdit->setText(item->getIconPath());
}

void AppConfigDialog::onSelectIconClicked()
{
    // 跨平台文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择应用图标"),
        QDir::currentPath(),
        tr("图片文件 (*.png *.jpg *.ico *.bmp *.svg)"));
    
    if (!fileName.isEmpty()) {
        ui->iconPathEdit->setText(fileName);
    }
}

void AppConfigDialog::onConfirmClicked()
{
    QString appName = ui->appNameEdit->text().trimmed();
    QString iconPath = ui->iconPathEdit->text().trimmed();
    
    // 校验应用名非空
    if (appName.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("应用名不能为空"));
        return;
    }
    
    // 如果指定了图标路径，检查文件是否存在
    if (!iconPath.isEmpty() && !QFile::exists(iconPath)) {
        QMessageBox::warning(this, tr("提示"), tr("图标文件不存在"));
        return;
    }
    
    // 创建新的应用项（图标路径可以为空，将使用默认图标）
    newApp = new AppItem(appName, iconPath, this);
    accept();
}

void AppConfigDialog::onCancelClicked()
{
    reject();
}

AppItem* AppConfigDialog::getNewApp() const
{
    return newApp;
}