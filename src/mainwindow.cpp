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
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QLocale>
#include <QHBoxLayout>
#include <yaml-cpp/yaml.h>

namespace {
enum ItemTag { TagNull = 0, TagAppItem = 1, TagFuncItem = 2 };

const QString DARK_OVERLAYS = QStringLiteral(
    "QMainWindow, QDialog, QWidget { background-color: #1e1e1e; color: #e0e0e0; }"
    "QDialog { background-color: #2d2d2d; }"
    "QMenuBar { background-color: #1565c0; }"
    "QMenu { background-color: #2d2d2d; border: 1px solid #444444; }"
    "QMenu::item { color: #e0e0e0; }"
    "QMenu::item:selected { background-color: #1a73e8; color: #ffffff; }"
    "QMenu::separator { background-color: #444444; }"
    "QListWidget#cmdListWidget { background-color: #252525; border: 1px solid #444444; color: #e0e0e0; }"
    "QListWidget#cmdListWidget::item { color: #e0e0e0; }"
    "QListWidget#cmdListWidget::item:hover { background-color: #1a3a6a; }"
    "QListWidget#cmdListWidget::item:selected { background-color: #1a3a6a; color: #90caf9; }"
    "QPushButton { background-color: #1565c0; }"
    "QPushButton:hover { background-color: #1976d2; }"
    "QPushButton#cancelBtn { background-color: #424242; color: #e0e0e0; border: 1px solid #555555; }"
    "QPushButton#cancelBtn:hover { background-color: #505050; }"
    "QPushButton#selectIconBtn { background-color: #333333; color: #90caf9; border: 1px solid #555555; }"
    "QPushButton#selectIconBtn:hover { background-color: #1a3a6a; }"
    "QLineEdit { background-color: #333333; border: 1.5px solid #555555; color: #e0e0e0; }"
    "QLineEdit:focus { border-color: #1565c0; }"
    "QLineEdit:read-only { background-color: #2a2a2a; }"
    "QGroupBox { color: #aaaaaa; border: 1.5px solid #444444; }"
    "QGroupBox::title { background-color: #2d2d2d; color: #90caf9; }"
    "QStatusBar { background-color: #252525; border-top: 1px solid #333333; color: #aaaaaa; }"
    "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #555555; }"
    "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: #666666; }"
    "QLabel { color: #cccccc; }"
    "QMessageBox { background-color: #2d2d2d; }"
    "QMessageBox QLabel { color: #e0e0e0; }"
    "QComboBox QAbstractItemView { background-color: #2d2d2d; color: #e0e0e0; selection-background-color: #1a3a6a; selection-color: #90caf9; }"
);
}

MainWindow::MainWindow(AppItem *initialItem, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , delegate(nullptr)
    , currentItem(initialItem)
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

    setWindowIcon(QIcon(":/resources/app_icon.ico"));
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
    actionToggleDarkMode = ui->actionToggleDarkMode;

    connect(actionOpen_config, &QAction::triggered, this, &MainWindow::onOpenConfig);
    connect(actionExit, &QAction::triggered, this, &MainWindow::onExit);
    connect(actionQuickStart, &QAction::triggered, this, &MainWindow::onAboutQuickStart);
    connect(actionQt, &QAction::triggered, this, &MainWindow::onAboutQt);
    connect(actionApp, &QAction::triggered, this, &MainWindow::onAddAppClicked);
    connect(actionFunc, &QAction::triggered, this, &MainWindow::onAddFuncClicked);
    connect(actionToggleDarkMode, &QAction::toggled, this, &MainWindow::onToggleDarkMode);

    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        m_lightStyleSheet = QString::fromUtf8(styleFile.readAll());
        styleFile.close();
    }

    loadConfig();

    if (!this->currentItem) {
        this->currentItem = rootItem;
    }

    setupLanguageToggle();

    if (m_darkMode) {
        actionToggleDarkMode->setChecked(true);
    }

    QString name = this->currentItem->getName().isEmpty() ? tr("Home") : this->currentItem->getName();
    setWindowTitle(tr("App Launcher - %1").arg(name));
    ui->statusbar->showMessage(tr("Current: %1").arg(name));

    connect(ui->iconListWidget, &QListWidget::itemClicked, this, &MainWindow::onIconListItemClicked);

    setupContextMenu();
    refreshIconList();
}

MainWindow::~MainWindow()
{
    if (currentTranslator) {
        qApp->removeTranslator(currentTranslator);
    }
    delete ui;
    delete rootItem;
    if (contextMenu) {
        delete contextMenu;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveConfig();
    MainWindow *parentMain = qobject_cast<MainWindow*>(parent());
    if (parentMain) {
        parentMain->refreshIconList();
        parentMain->show();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange && currentItem) {
        ui->retranslateUi(this);
        retranslateLanguageToggle();
        QString name = currentItem->getName().isEmpty() ? tr("Home") : currentItem->getName();
        setWindowTitle(tr("App Launcher - %1").arg(name));
        ui->statusbar->showMessage(tr("Current: %1").arg(name));
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupLanguageToggle()
{
    languageLabel = new QLabel(this);
    languageCombo = new QComboBox(this);
    languageCombo->addItem("English", QString("en"));
    languageCombo->addItem("中文", QString("zh_CN"));

    QString lang;
    if (!m_savedLanguage.isEmpty()) {
        lang = m_savedLanguage;
    } else {
        lang = (QLocale::system().name() == "zh_CN") ? "zh_CN" : "en";
    }
    int defaultIndex = (lang == "zh_CN") ? 1 : 0;
    languageCombo->setCurrentIndex(defaultIndex);

    connect(languageCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onLanguageChanged);

    QWidget *langContainer = new QWidget(this);
    QHBoxLayout *langLayout = new QHBoxLayout(langContainer);
    langLayout->setContentsMargins(0, 0, 8, 0);
    langLayout->addWidget(languageLabel);
    langLayout->addWidget(languageCombo);
    langContainer->setStyleSheet(
        "QLabel { color: #ffffff; font-size: 12px; }"
        "QComboBox { background-color: rgba(255,255,255,0.15); color: #ffffff; "
        "  border: 1px solid rgba(255,255,255,0.3); border-radius: 4px; "
        "  padding: 2px 6px; font-size: 12px; min-width: 80px; }"
        "QComboBox:hover { background-color: rgba(255,255,255,0.25); }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #ffffff; color: #2c3e50; "
        "  selection-background-color: #e8f0fe; selection-color: #1a73e8; }"
    );
    ui->menubar->setCornerWidget(langContainer, Qt::TopRightCorner);

    retranslateLanguageToggle();

    QTranslator *initialTranslator = new QTranslator(this);
    if (initialTranslator->load(":/i18n/QuickStart_" + lang)) {
        currentTranslator = initialTranslator;
        qApp->installTranslator(initialTranslator);
    } else {
        delete initialTranslator;
    }
}

void MainWindow::retranslateLanguageToggle()
{
    languageLabel->setText(tr("Language:"));
    languageCombo->setItemText(0, tr("English"));
    languageCombo->setItemText(1, tr("Chinese"));
}

void MainWindow::onLanguageChanged(int index)
{
    QString locale = languageCombo->itemData(index).toString();

    QTranslator *translator = new QTranslator(this);
    if (translator->load(":/i18n/QuickStart_" + locale)) {
        if (currentTranslator) {
            qApp->removeTranslator(currentTranslator);
            delete currentTranslator;
        }
        currentTranslator = translator;
        qApp->installTranslator(translator);
        saveConfig();
    } else {
        delete translator;
    }
}

void MainWindow::onToggleDarkMode(bool checked)
{
    m_darkMode = checked;
    if (checked) {
        qApp->setStyleSheet(m_lightStyleSheet + "\n" + DARK_OVERLAYS);
    } else {
        qApp->setStyleSheet(m_lightStyleSheet);
    }
    saveConfig();
}

void MainWindow::setupContextMenu()
{
    contextMenu = new QMenu(this);

    editAction = new QAction(tr("Edit"), this);
    deleteAction = new QAction(tr("Delete"), this);

    connect(editAction, &QAction::triggered, this, &MainWindow::onEditItem);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteItem);

    contextMenu->addAction(editAction);
    contextMenu->addAction(deleteAction);

    ui->iconListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->iconListWidget, &QWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    contextMenuItem = ui->iconListWidget->itemAt(pos);

    if (contextMenuItem) {
        int tag = contextMenuItem->data(Qt::UserRole + 1).toInt();
        if (tag == TagNull) {
            return;
        }
        contextMenu->exec(ui->iconListWidget->mapToGlobal(pos));
    }
}

void MainWindow::onEditItem()
{
    if (!contextMenuItem || !currentItem) return;

    int tag = contextMenuItem->data(Qt::UserRole + 1).toInt();
    if (tag == TagAppItem) {
        auto *appItem = static_cast<AppItem*>(contextMenuItem->data(Qt::UserRole).value<QObject*>());
        if (currentItem->getSubApps().contains(appItem)) {
            AppConfigDialog dialog(appItem, this);
            if (dialog.exec() == QDialog::Accepted) {
                AppItem *newApp = dialog.getNewApp();
                if (newApp) {
                    int index = currentItem->getSubApps().indexOf(appItem);
                    if (index != -1) {
                        currentItem->removeSubApp(appItem);
                        currentItem->addSubApp(newApp);
                        contextMenuItem = nullptr;
                        refreshIconList();
                        saveConfig();
                    }
                }
            }
        }
    }
    else if (tag == TagFuncItem) {
        auto *funcItem = static_cast<FuncItem*>(contextMenuItem->data(Qt::UserRole).value<QObject*>());
        if (currentItem->getFuncs().contains(funcItem)) {
            FuncConfigDialog dialog(funcItem, this);
            if (dialog.exec() == QDialog::Accepted) {
                FuncItem *newFunc = dialog.getNewFunc();
                if (newFunc) {
                    int index = currentItem->getFuncs().indexOf(funcItem);
                    if (index != -1) {
                        currentItem->removeFunc(funcItem);
                        currentItem->addFunc(newFunc);
                        contextMenuItem = nullptr;
                        refreshIconList();
                        saveConfig();
                    }
                }
            }
        }
    }
}

void MainWindow::onDeleteItem()
{
    if (!contextMenuItem || !currentItem) return;

    int tag = contextMenuItem->data(Qt::UserRole + 1).toInt();
    if (tag == TagNull) return;

    QString itemName = contextMenuItem->text();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Delete"),
                                 tr("Are you sure you want to delete \"%1\"?").arg(itemName),
                                 QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (tag == TagAppItem) {
            auto *appItem = static_cast<AppItem*>(contextMenuItem->data(Qt::UserRole).value<QObject*>());
            if (currentItem->getSubApps().contains(appItem)) {
                currentItem->removeSubApp(appItem);
                contextMenuItem = nullptr;
                refreshIconList();
                saveConfig();
                return;
            }
        }
        else if (tag == TagFuncItem) {
            auto *funcItem = static_cast<FuncItem*>(contextMenuItem->data(Qt::UserRole).value<QObject*>());
            if (currentItem->getFuncs().contains(funcItem)) {
                currentItem->removeFunc(funcItem);
                contextMenuItem = nullptr;
                refreshIconList();
                saveConfig();
                return;
            }
        }
    }
}

void MainWindow::loadConfig()
{
    QFile yamlFile(AppConfig::CONFIG_FILE_PATH_YAML);
    if (yamlFile.exists() && yamlFile.open(QIODevice::ReadOnly)) {
        QByteArray data = yamlFile.readAll();
        yamlFile.close();

        try {
            YAML::Node rootNode = YAML::Load(data.toStdString());
            if (rootNode && rootNode.IsMap()) {
                if (rootNode["language"]) {
                    m_savedLanguage = QString::fromStdString(rootNode["language"].as<std::string>());
                }
                if (rootNode["theme"]) {
                    QString theme = QString::fromStdString(rootNode["theme"].as<std::string>());
                    m_darkMode = (theme == "dark");
                }
                auto *newRoot = new AppItem();
                newRoot->fromYaml(rootNode);
                rootItem = newRoot;
                return;
            }
        } catch (const YAML::Exception &) {
        }
    }

    QFile jsonFile(AppConfig::CONFIG_FILE_PATH);
    if (jsonFile.exists() && jsonFile.open(QIODevice::ReadOnly)) {
        QByteArray data = jsonFile.readAll();
        jsonFile.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            rootItem = new AppItem();
            rootItem->fromJson(doc.object());
        } else {
            rootItem = new AppItem("Home", "");
        }
    } else {
        rootItem = new AppItem("Home", "");
    }
}

void MainWindow::saveConfig()
{
    if (!rootItem) return;

    YAML::Node rootNode = rootItem->toYaml();
    if (languageCombo) {
        rootNode["language"] = languageCombo->currentData().toString().toStdString();
    }
    rootNode["theme"] = m_darkMode ? "dark" : "light";
    YAML::Emitter emitter;
    emitter.SetIndent(4);
    emitter << rootNode;
    if (!emitter.good()) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to serialize config: %1").arg(emitter.GetLastError().c_str()));
        return;
    }

    QFile configFile(AppConfig::CONFIG_FILE_PATH_YAML);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(emitter.c_str());
        configFile.close();
    } else {
        QMessageBox::warning(this, tr("Notice"), tr("Failed to save config file, please check file permissions"));
    }
}

void MainWindow::refreshIconList()
{
    if (!currentItem) return;

    ui->iconListWidget->clear();

    for (AppItem *app : currentItem->getSubApps()) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(app->getName());
        item->setIcon(app->getIcon());
        item->setData(Qt::UserRole, QVariant::fromValue<QObject*>(app));
        item->setData(Qt::UserRole + 1, TagAppItem);
        ui->iconListWidget->addItem(item);
    }

    for (FuncItem *func : currentItem->getFuncs()) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(func->getName());
        item->setIcon(func->getIcon());
        item->setData(Qt::UserRole, QVariant::fromValue<QObject*>(func));
        item->setData(Qt::UserRole + 1, TagFuncItem);
        ui->iconListWidget->addItem(item);
    }

    QListWidgetItem *addItem = new QListWidgetItem();
    addItem->setText(tr("+ Add"));
    addItem->setIcon(IconGenerator::generateIcon("+", Qt::lightGray, Qt::white, 64));
    addItem->setData(Qt::UserRole, QVariant::fromValue<QObject*>(nullptr));
    addItem->setData(Qt::UserRole + 1, TagNull);
    ui->iconListWidget->addItem(addItem);
}

void MainWindow::onIconListItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    int tag = item->data(Qt::UserRole + 1).toInt();

    if (tag == TagNull) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Select Type"));
        msgBox.setText(tr("Please select the type to add:"));

        QPushButton *appBtn = msgBox.addButton(tr("Add App"), QMessageBox::ActionRole);
        QPushButton *funcBtn = msgBox.addButton(tr("Add Function"), QMessageBox::ActionRole);
        msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);

        msgBox.exec();

        QAbstractButton *clickedBtn = msgBox.clickedButton();
        if (clickedBtn == appBtn) {
            onAddAppClicked();
        } else if (clickedBtn == funcBtn) {
            onAddFuncClicked();
        }
        return;
    }
    else if (tag == TagAppItem) {
        auto *appItem = static_cast<AppItem*>(item->data(Qt::UserRole).value<QObject*>());
        if (currentItem->getSubApps().contains(appItem)) {
            MainWindow *newWindow = new MainWindow(appItem, this);
            newWindow->setAttribute(Qt::WA_DeleteOnClose);
            newWindow->show();
            this->hide();
        }
    }
    else if (tag == TagFuncItem) {
        auto *funcItem = static_cast<FuncItem*>(item->data(Qt::UserRole).value<QObject*>());
        for (const QString &cmd : funcItem->getCmds()) {
            QStringList parts = QProcess::splitCommand(cmd);
            if (!parts.isEmpty()) {
                QString program = parts.first();
                QStringList args = parts.mid(1);
                if (!QProcess::startDetached(program, args)) {
                    QMessageBox::warning(this, tr("Launch Error"),
                        tr("Failed to launch: %1").arg(program));
                }
            }
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

void MainWindow::onOpenConfig()
{
    QString configPath = QDir::current().absoluteFilePath(AppConfig::CONFIG_FILE_PATH_YAML);
    QFileInfo fileInfo(configPath);

    if (!fileInfo.exists()) {
        configPath = QDir::current().absoluteFilePath(AppConfig::CONFIG_FILE_PATH);
        fileInfo = QFileInfo(configPath);
    }

    if (fileInfo.exists()) {
        QUrl fileUrl = QUrl::fromLocalFile(configPath);
        QDesktopServices::openUrl(fileUrl);
    } else {
        QMessageBox::warning(this, tr("Open Config"),
            tr("Config file not found: %1").arg(configPath));
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onAboutQuickStart()
{
    QMessageBox::about(this, tr("About QuickStart"),
        AppConfig::aboutHtml());
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
