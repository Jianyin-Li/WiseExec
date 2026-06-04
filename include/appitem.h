#ifndef APPITEM_H
#define APPITEM_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QIcon>
#include "funcitem.h"

class AppItem : public QObject
{
    Q_OBJECT

public:
    explicit AppItem(QObject *parent = nullptr);
    explicit AppItem(const QString &name, const QString &iconPath, QObject *parent = nullptr);
    ~AppItem();

    QString getName() const { return name; }
    void setName(const QString &newName) { name = newName; }
    
    QString getIconPath() const { return iconPath; }
    void setIconPath(const QString &newIconPath) { iconPath = newIconPath; }
    
    QIcon getIcon() const;
    
    QList<AppItem*> getSubApps() const { return subApps; }
    QList<FuncItem*> getFuncs() const { return funcs; }
    
    void addSubApp(AppItem *app);
    void addFunc(FuncItem *func);
    void removeSubApp(AppItem *app);
    void removeFunc(FuncItem *func);
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

private:
    QString name;
    QString iconPath;
    QList<AppItem*> subApps;
    QList<FuncItem*> funcs;
};

#endif // APPITEM_H