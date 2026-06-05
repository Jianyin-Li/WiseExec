#include "funcitem.h"
#include "icongenerator.h"
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <yaml-cpp/yaml.h>

FuncItem::FuncItem(QObject *parent)
    : QObject(parent)
    , name("")
    , iconPath("")
{
}

FuncItem::FuncItem(const QString &name, const QString &iconPath, const QStringList &cmds, QObject *parent)
    : QObject(parent)
    , name(name)
    , iconPath(iconPath)
    , cmds(cmds)
{
}

QIcon FuncItem::getIcon() const
{
    if (!iconPath.isEmpty()) {
        QFileInfo fileInfo(iconPath);
        if (fileInfo.exists()) {
            return QIcon(iconPath);
        }
    }

    return IconGenerator::generateDefaultIcon(name);
}

void FuncItem::addCmd(const QString &cmd)
{
    if (!cmd.isEmpty() && !cmds.contains(cmd)) {
        cmds.append(cmd);
    }
}

void FuncItem::removeCmd(int index)
{
    if (index >= 0 && index < cmds.size()) {
        cmds.removeAt(index);
    }
}

QJsonObject FuncItem::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["iconPath"] = iconPath;
    
    QJsonArray cmdsArray;
    for (const QString &cmd : cmds) {
        cmdsArray.append(cmd);
    }
    obj["cmds"] = cmdsArray;
    
    return obj;
}

void FuncItem::fromJson(const QJsonObject &obj)
{
    name = obj["name"].toString();
    iconPath = obj["iconPath"].toString();
    
    cmds.clear();
    QJsonArray cmdsArray = obj["cmds"].toArray();
    for (const QJsonValue &value : cmdsArray) {
        cmds.append(value.toString());
    }
}

YAML::Node FuncItem::toYaml() const
{
    YAML::Node node;
    node["name"] = name.toStdString();
    node["iconPath"] = iconPath.toStdString();

    for (const QString &cmd : cmds) {
        node["cmds"].push_back(cmd.toStdString());
    }

    return node;
}

void FuncItem::fromYaml(const YAML::Node &node)
{
    if (node["name"]) {
        name = QString::fromStdString(node["name"].as<std::string>());
    }
    if (node["iconPath"]) {
        iconPath = QString::fromStdString(node["iconPath"].as<std::string>());
    }

    cmds.clear();
    if (node["cmds"] && node["cmds"].IsSequence()) {
        for (const auto &cmd : node["cmds"]) {
            cmds.append(QString::fromStdString(cmd.as<std::string>()));
        }
    }
}