#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QEvent>
#include <QTranslator>
#include <QComboBox>
#include <QLabel>
#include "appitem.h"
#include "appconfigdialog.h"
#include "funcconfigdialog.h"
#include "config.h"
#include "iconlistdelegate.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppItem *currentItem = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onIconListItemClicked(QListWidgetItem *item);
    void onAddAppClicked();
    void onAddFuncClicked();
    void showContextMenu(const QPoint &pos);
    void onEditItem();
    void onDeleteItem();
    void onOpenConfig();
    void onExit();
    void onAboutQuickStart();
    void onAboutQt();
    void onLanguageChanged(int index);

private:
    void refreshIconList();
    void saveConfig();
    void loadConfig();
    void setupContextMenu();
    void setupLanguageToggle();
    void retranslateLanguageToggle();

    Ui::MainWindow *ui;
    IconListDelegate *delegate;
    AppItem *currentItem;
    AppItem *rootItem;
    const QString CONFIG_FILE_PATH = "./config.json";
    QTranslator *currentTranslator = nullptr;

    QMenu *contextMenu;
    QAction *editAction;
    QAction *deleteAction;
    QListWidgetItem *contextMenuItem;

    QAction *actionOpen_config;
    QAction *actionExit;
    QAction *actionQuickStart;
    QAction *actionQt;
    QAction *actionApp;
    QAction *actionFunc;

    QLabel *languageLabel = nullptr;
    QComboBox *languageCombo = nullptr;
};
#endif // MAINWINDOW_H

