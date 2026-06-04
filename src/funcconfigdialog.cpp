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

    connect(ui->selectIconBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectIconClicked);
    connect(ui->addCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onAddCmdClicked);
    connect(ui->delCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onDelCmdClicked);
    connect(ui->selectExeBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectExeClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &FuncConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &FuncConfigDialog::onCancelClicked);

    ui->iconPathEdit->setReadOnly(true);
}

FuncConfigDialog::FuncConfigDialog(FuncItem *existingItem, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FuncConfigDialog)
    , newFunc(nullptr)
    , existingItem(existingItem)
{
    ui->setupUi(this);

    connect(ui->selectIconBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectIconClicked);
    connect(ui->addCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onAddCmdClicked);
    connect(ui->delCmdBtn, &QPushButton::clicked, this, &FuncConfigDialog::onDelCmdClicked);
    connect(ui->selectExeBtn, &QPushButton::clicked, this, &FuncConfigDialog::onSelectExeClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &FuncConfigDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &FuncConfigDialog::onCancelClicked);

    ui->iconPathEdit->setReadOnly(true);

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

    ui->cmdListWidget->clear();
    for (const QString &cmd : item->getCmds()) {
        ui->cmdListWidget->addItem(cmd);
    }
}

void FuncConfigDialog::onSelectIconClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Function Icon"),
        QDir::currentPath(),
        tr("Image Files (*.png *.jpg *.ico *.bmp *.svg)"));

    if (!fileName.isEmpty()) {
        ui->iconPathEdit->setText(fileName);
    }
}

void FuncConfigDialog::onAddCmdClicked()
{
    bool ok;
    QString command = QInputDialog::getText(this,
        tr("Enter Command"),
        tr("Enter the command to execute (e.g. notepad.exe, calc.exe):"),
        QLineEdit::Normal,
        "",
        &ok);

    if (ok && !command.trimmed().isEmpty()) {
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
            QMessageBox::information(this, tr("Notice"), tr("This command already exists"));
        }
    }
}

void FuncConfigDialog::onSelectExeClicked()
{
    QString filter;
#ifdef Q_OS_WIN
    filter = tr("Executables (*.exe *.bat *.cmd);;All Files (*.*)");
#else
    filter = tr("Executables (*.sh *.bin);;All Files (*)");
#endif

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Executable"),
        QDir::currentPath(),
        filter);

    if (!fileName.isEmpty()) {
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
            QMessageBox::information(this, tr("Notice"), tr("This file already exists"));
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

    if (funcName.isEmpty()) {
        QMessageBox::warning(this, tr("Notice"), tr("Function name cannot be empty"));
        return;
    }

    if (!iconPath.isEmpty() && !QFile::exists(iconPath)) {
        QMessageBox::warning(this, tr("Notice"), tr("Icon file does not exist"));
        return;
    }

    if (ui->cmdListWidget->count() == 0) {
        QMessageBox::warning(this, tr("Notice"), tr("Command list cannot be empty"));
        return;
    }

    QStringList cmds;
    for (int i = 0; i < ui->cmdListWidget->count(); ++i) {
        cmds.append(ui->cmdListWidget->item(i)->text());
    }

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
