#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QCloseEvent>
#include "appitem.h"
#include "appconfigdialog.h"
#include "funcconfigdialog.h"
#include "config.h"

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

private slots:
    void onIconListItemClicked(QListWidgetItem *item);
    void onAddAppClicked();
    void onAddFuncClicked();
    void showContextMenu(const QPoint &pos);
    void onEditItem();
    void onDeleteItem();
    //void onNewAction();
    void onOpenConfig();
    void onExit();
    void onAboutQuickStart();
    void onAboutQt();

private:
    void refreshIconList();
    void saveConfig();
    void loadConfig();
    void setupContextMenu();

    Ui::MainWindow *ui;
    AppItem *currentItem;
    AppItem *rootItem;
    const QString CONFIG_FILE_PATH = "./config.json";

    // 右键菜单相关
    QMenu *contextMenu;
    QAction *editAction;
    QAction *deleteAction;
    QListWidgetItem *contextMenuItem;

    // 主菜单动作
    //QAction *actionNew;
    QAction *actionOpen_config;
    QAction *actionExit;
    QAction *actionQuickStart;
    QAction *actionQt;
    QAction *actionApp;
    QAction *actionFunc;
};
#endif // MAINWINDOW_H

