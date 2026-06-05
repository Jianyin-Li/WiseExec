#include "appitem.h"
#include "icongenerator.h"
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>

AppItem::AppItem(QObject *parent)
    : QObject(parent)
    , name("")
    , iconPath("")
{
}

AppItem::AppItem(const QString &name, const QString &iconPath, QObject *parent)
    : QObject(parent)
    , name(name)
    , iconPath(iconPath)
{
}

AppItem::~AppItem()
{
}

QIcon AppItem::getIcon() const
{
    if (!iconPath.isEmpty()) {
        QFileInfo fileInfo(iconPath);
        if (fileInfo.exists()) {
            return QIcon(iconPath);
        }
    }

    return IconGenerator::generateDefaultIcon(name);
}

void AppItem::addSubApp(AppItem *app)
{
    if (app) {
        app->setParent(this);
        subApps.append(app);
    }
}

void AppItem::addFunc(FuncItem *func)
{
    if (func) {
        func->setParent(this);
        funcs.append(func);
    }
}

void AppItem::removeSubApp(AppItem *app)
{
    if (app) {
        subApps.removeAll(app);
        app->deleteLater();
    }
}

void AppItem::removeFunc(FuncItem *func)
{
    if (func) {
        funcs.removeAll(func);
        func->deleteLater();
    }
}

QJsonObject AppItem::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["iconPath"] = iconPath;
    
    QJsonArray subAppsArray;
    for (const AppItem *app : subApps) {
        subAppsArray.append(app->toJson());
    }
    obj["subApps"] = subAppsArray;
    
    QJsonArray funcsArray;
    for (const FuncItem *func : funcs) {
        funcsArray.append(func->toJson());
    }
    obj["funcs"] = funcsArray;
    
    return obj;
}

void AppItem::fromJson(const QJsonObject &obj)
{
    name = obj["name"].toString();
    iconPath = obj["iconPath"].toString();

    for (AppItem *app : subApps) {
        app->deleteLater();
    }
    for (FuncItem *func : funcs) {
        func->deleteLater();
    }
    subApps.clear();
    funcs.clear();

    QJsonArray subAppsArray = obj["subApps"].toArray();
    for (const QJsonValue &value : subAppsArray) {
        AppItem *app = new AppItem(this);
        app->fromJson(value.toObject());
        subApps.append(app);
    }

    QJsonArray funcsArray = obj["funcs"].toArray();
    for (const QJsonValue &value : funcsArray) {
        FuncItem *func = new FuncItem(this);
        func->fromJson(value.toObject());
        funcs.append(func);
    }
}