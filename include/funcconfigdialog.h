#ifndef FUNCCONFIGDIALOG_H
#define FUNCCONFIGDIALOG_H

#include <QDialog>
#include "funcitem.h"

namespace Ui {
class FuncConfigDialog;
}

class FuncConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FuncConfigDialog(QWidget *parent = nullptr);
    explicit FuncConfigDialog(FuncItem *existingItem, QWidget *parent = nullptr);
    ~FuncConfigDialog();
    
    FuncItem* getNewFunc() const;

private slots:
    void onSelectIconClicked();
    void onAddCmdClicked();
    void onSelectExeClicked();
    void onDelCmdClicked();
    void onConfirmClicked();
    void onCancelClicked();

private:
    void initFromItem(FuncItem *item);
    
    Ui::FuncConfigDialog *ui;
    FuncItem *newFunc;
    FuncItem *existingItem;
};

#endif // FUNCCONFIGDIALOG_H