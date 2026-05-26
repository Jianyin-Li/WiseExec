#include "mainwindow.h"
#include "../ui/ui_mainwindow.h"
#include "icongenerator.h"

#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QIcon>  // 添加QIcon头文件
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(AppItem *currentItem, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , delegate(nullptr)
    , currentItem(currentItem)
    , rootItem(nullptr)
    , contextMenu(nullptr)
    , editAction(nullptr)
    , deleteAction(nullptr)
    , contextMenuItem(nullptr)
    , actionOpen_config(nullptr)
    , actionExit(nullptr)
    , actionQuickStart(nullptr)
    , actionQt(nullptr)
    , actionApp(nullptr)
    , actionFunc(nullptr)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/app_icon.ico"));
    setMinimumSize(640, 480);

    delegate = new IconListDelegate(this);
    ui->iconListWidget->setItemDelegate(delegate);
    ui->iconListWidget->setMouseTracking(true);
    ui->iconListWidget->setGridSize(QSize(120, 130));
    ui->iconListWidget->setIconSize(QSize(56, 56));
    ui->iconListWidget->setWordWrap(true);
    ui->iconListWidget->setResizeMode(QListView::Adjust);
    ui->iconListWidget->setMovement(QListView::Static);
    ui->iconListWidget->setViewMode(QListView::IconMode);
    ui->iconListWidget->setSpacing(6);

    actionOpen_config = ui->actionOpen_config;
    actionExit = ui->actionExit;
    actionQuickStart = ui->actionQuickStart;
    actionQt = ui->actionQt;
    actionApp = ui->actionApp;
    actionFunc = ui->actionFunc;

    connect(actionOpen_config, &QAction::triggered, this, &MainWindow::onOpenConfig);
    connect(actionExit, &QAction::triggered, this, &MainWindow::onExit);
    connect(actionQuickStart, &QAction::triggered, this, &MainWindow::onAboutQuickStart);
    connect(actionQt, &QAction::triggered, this, &MainWindow::onAboutQt);
    connect(actionApp, &QAction::triggered, this, &MainWindow::onAddAppClicked);
    connect(actionFunc, &QAction::triggered, this, &MainWindow::onAddFuncClicked);

    loadConfig();

    if (!this->currentItem) {
        this->currentItem = rootItem;
    }

    setWindowTitle(QString("应用启动器 - %1").arg(this->currentItem->getName().isEmpty() ? "主界面" : this->currentItem->getName()));

    connect(ui->iconListWidget, &QListWidget::itemClicked, this, &MainWindow::onIconListItemClicked);

    setupContextMenu();
    refreshIconList();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (contextMenu) {
        delete contextMenu;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MainWindow *parentMain = qobject_cast<MainWindow*>(parent());
    if (parentMain) {
        parentMain->show();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupContextMenu()
{
    // 创建右键菜单
    contextMenu = new QMenu(this);
    
    // 创建菜单项
    editAction = new QAction(tr("编辑"), this);
    deleteAction = new QAction(tr("删除"), this);
    
    // 连接菜单项信号
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditItem);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteItem);
    
    // 添加到菜单
    contextMenu->addAction(editAction);
    contextMenu->addAction(deleteAction);
    
    // 设置列表控件的上下文菜单策略
    ui->iconListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->iconListWidget, &QWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    // 获取点击位置的项
    contextMenuItem = ui->iconListWidget->itemAt(pos);
    
    if (contextMenuItem) {
        // 检查是否是"+添加"项
        void *data = contextMenuItem->data(Qt::UserRole).value<void*>();
        if (!data) {
            // "+添加"项不显示右键菜单
            return;
        }
        
        // 显示右键菜单
        contextMenu->exec(ui->iconListWidget->mapToGlobal(pos));
    }
}

void MainWindow::onEditItem()
{
    if (!contextMenuItem || !currentItem) return;
    
    void *data = contextMenuItem->data(Qt::UserRole).value<void*>();
    if (!data) return;
    
    // 检查是AppItem还是FuncItem
    AppItem *appItem = reinterpret_cast<AppItem*>(data);
    FuncItem *funcItem = reinterpret_cast<FuncItem*>(data);
    
    // 尝试编辑AppItem
    if (appItem && currentItem->getSubApps().contains(appItem)) {
        AppConfigDialog dialog(appItem, this);  // 使用带编辑功能的构造函数
        if (dialog.exec() == QDialog::Accepted) {
            AppItem *newApp = dialog.getNewApp();
            if (newApp) {
                // 替换原有的项
                int index = currentItem->getSubApps().indexOf(appItem);
                if (index != -1) {
                    // 删除旧的
                    currentItem->removeSubApp(appItem);
                    // 添加新的
                    currentItem->addSubApp(newApp);
                    refreshIconList();
                    saveConfig();
                }
            }
        }
    } 
    // 尝试编辑FuncItem
    else if (funcItem && currentItem->getFuncs().contains(funcItem)) {
        FuncConfigDialog dialog(funcItem, this);  // 使用带编辑功能的构造函数
        if (dialog.exec() == QDialog::Accepted) {
            FuncItem *newFunc = dialog.getNewFunc();
            if (newFunc) {
                // 替换原有的项
                int index = currentItem->getFuncs().indexOf(funcItem);
                if (index != -1) {
                    // 删除旧的
                    currentItem->removeFunc(funcItem);
                    // 添加新的
                    currentItem->addFunc(newFunc);
                    refreshIconList();
                    saveConfig();
                }
            }
        }
    }
}

void MainWindow::onDeleteItem()
{
    if (!contextMenuItem || !currentItem) return;
    
    void *data = contextMenuItem->data(Qt::UserRole).value<void*>();
    if (!data) return;
    
    QString itemName = contextMenuItem->text();
    
    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("确认删除"),
                                 tr("确定要删除 \"%1\" 吗？").arg(itemName),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 检查是AppItem还是FuncItem
        AppItem *appItem = reinterpret_cast<AppItem*>(data);
        FuncItem *funcItem = reinterpret_cast<FuncItem*>(data);
        
        // 删除AppItem
        if (appItem && currentItem->getSubApps().contains(appItem)) {
            currentItem->removeSubApp(appItem);
            refreshIconList();
            saveConfig();
        }
        // 删除FuncItem
        else if (funcItem && currentItem->getFuncs().contains(funcItem)) {
            currentItem->removeFunc(funcItem);
            refreshIconList();
            saveConfig();
        }
    }
}

void MainWindow::loadConfig()
{
    QFile configFile(CONFIG_FILE_PATH);
    if (configFile.exists() && configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        configFile.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            rootItem = new AppItem();
            rootItem->fromJson(doc.object());
        } else {
            // 配置文件格式错误，创建空根节点
            rootItem = new AppItem("主界面", "");
        }
    } else {
        // 配置文件不存在，创建空根节点
        rootItem = new AppItem("主界面", "");
    }
}

void MainWindow::saveConfig()
{
    if (!rootItem) return;
    
    QJsonObject rootObj = rootItem->toJson();
    QJsonDocument doc(rootObj);
    
    QFile configFile(CONFIG_FILE_PATH);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
    } else {
        QMessageBox::warning(this, tr("提示"), tr("保存配置文件失败，请检查文件权限"));
    }
}

void MainWindow::refreshIconList()
{
    if (!currentItem) return;
    
    ui->iconListWidget->clear();
    
    // 添加子应用项
    for (AppItem *app : currentItem->getSubApps()) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(app->getName());
        item->setIcon(app->getIcon());  // 使用getIcon方法
        item->setData(Qt::UserRole, QVariant::fromValue<void*>(static_cast<void*>(app)));
        ui->iconListWidget->addItem(item);
    }
    
    // 添加功能项
    for (FuncItem *func : currentItem->getFuncs()) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(func->getName());
        item->setIcon(func->getIcon());  // 使用getIcon方法
        item->setData(Qt::UserRole, QVariant::fromValue<void*>(static_cast<void*>(func)));
        ui->iconListWidget->addItem(item);
    }
    
    // 添加+添加项
    QListWidgetItem *addItem = new QListWidgetItem();
    addItem->setText("+添加");
    addItem->setIcon(IconGenerator::generateIcon("+", Qt::lightGray, Qt::white, 64));
    addItem->setData(Qt::UserRole, QVariant::fromValue<void*>(nullptr));
    ui->iconListWidget->addItem(addItem);
}

void MainWindow::onIconListItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    void *data = item->data(Qt::UserRole).value<void*>();
    
    if (!data) {
        // +添加项被点击
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("选择添加类型"));
        msgBox.setText(tr("请选择要添加的类型："));
        
        // 存储按钮指针以便比较
        QPushButton *appBtn = msgBox.addButton(tr("添加应用"), QMessageBox::ActionRole);
        QPushButton *funcBtn = msgBox.addButton(tr("添加功能"), QMessageBox::ActionRole);
        msgBox.addButton(tr("取消"), QMessageBox::RejectRole);
        
        msgBox.exec();
        
        QAbstractButton *clickedBtn = msgBox.clickedButton();
        if (clickedBtn == appBtn) {
            onAddAppClicked();
        } else if (clickedBtn == funcBtn) {
            onAddFuncClicked();
        }
        return;
    }
    
    // 检查是AppItem还是FuncItem
    AppItem *appItem = reinterpret_cast<AppItem*>(data);
    FuncItem *funcItem = reinterpret_cast<FuncItem*>(data);
    
    // 尝试转换为AppItem
    if (appItem && currentItem->getSubApps().contains(appItem)) {
        // 打开新的MainWindow显示该应用项
        MainWindow *newWindow = new MainWindow(appItem, this);
        newWindow->show();
        this->hide();
    } 
    // 尝试转换为FuncItem
    else if (funcItem && currentItem->getFuncs().contains(funcItem)) {
        // 运行所有命令
        for (const QString &cmd : funcItem->getCmds()) {
            QProcess::startDetached(cmd);
        }
    }
}

void MainWindow::onAddAppClicked()
{
    AppConfigDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        AppItem *newApp = dialog.getNewApp();
        if (newApp) {
            currentItem->addSubApp(newApp);
            refreshIconList();
            saveConfig();
        }
    }
}

void MainWindow::onAddFuncClicked()
{
    FuncConfigDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        FuncItem *newFunc = dialog.getNewFunc();
        if (newFunc) {
            currentItem->addFunc(newFunc);
            refreshIconList();
            saveConfig();
        }
    }
}

/*
void MainWindow::onNewAction()
{
    // 导航回到主界面（根节点）
    if (currentItem != rootItem) {
        currentItem = rootItem;
        refreshIconList();
        setWindowTitle(QString("应用启动器 - 主界面"));
    }
}
*/


void MainWindow::onOpenConfig()
{
    // 打开配置文件
    QString configPath = QDir::current().absoluteFilePath(CONFIG_FILE_PATH);
    QFileInfo fileInfo(configPath);
    
    if (fileInfo.exists()) {
        // 使用系统默认程序打开配置文件
        QUrl fileUrl = QUrl::fromLocalFile(configPath);
        QDesktopServices::openUrl(fileUrl);
    } else {
        QMessageBox::warning(this, tr("打开配置"), 
            tr("配置文件不存在：%1").arg(configPath));
    }
}

void MainWindow::onExit()
{
    // 退出
    QApplication::quit();
}

void MainWindow::onAboutQuickStart()
{
    // ����QuickStart
        QMessageBox::about(this, tr("关于QuickStart"), 
        AppConfig::aboutHtml());
}

void MainWindow::onAboutQt()
{
    // 关于Qt
    QMessageBox::aboutQt(this, tr("关于Qt"));
}
