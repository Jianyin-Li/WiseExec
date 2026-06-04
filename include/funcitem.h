#ifndef FUNCITEM_H
#define FUNCITEM_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QIcon>

class FuncItem : public QObject
{
    Q_OBJECT

public:
    explicit FuncItem(QObject *parent = nullptr);
    explicit FuncItem(const QString &name, const QString &iconPath, const QStringList &cmds, QObject *parent = nullptr);
    
    QString getName() const { return name; }
    void setName(const QString &newName) { name = newName; }
    
    QString getIconPath() const { return iconPath; }
    void setIconPath(const QString &newIconPath) { iconPath = newIconPath; }
    
    QIcon getIcon() const;
    
    QStringList getCmds() const { return cmds; }
    void setCmds(const QStringList &newCmds) { cmds = newCmds; }
    void addCmd(const QString &cmd);
    void removeCmd(int index);
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

private:
    QString name;
    QString iconPath;
    QStringList cmds;
};

#endif // FUNCITEM_H