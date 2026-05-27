#ifndef ICONGENERATOR_H
#define ICONGENERATOR_H

#include <QObject>
#include <QIcon>
#include <QPixmap>
#include <QColor>
#include <QString>

class IconGenerator : public QObject
{
    Q_OBJECT

public:
    explicit IconGenerator(QObject *parent = nullptr);
    
    // 生成默认图标（基于名称的第一个字母）
    static QIcon generateDefaultIcon(const QString &name, int size = 64);
    
    // 生成自定义图标
    static QIcon generateIcon(const QString &text, const QColor &backgroundColor, 
                             const QColor &textColor = Qt::white, int size = 64);
    
    // 获取颜色列表
    static const QList<QColor>& getDefaultColors();
    
    // 根据名称获取确定性颜色
    static QColor getColorForName(const QString &name);
    
private:
    // 绘制圆形图标
    static QPixmap drawCircularIcon(const QString &text, const QColor &backgroundColor,
                                   const QColor &textColor, int size);
    
    // 绘制方形图标
    static QPixmap drawSquareIcon(const QString &text, const QColor &backgroundColor,
                                 const QColor &textColor, int size);
};

#endif // ICONGENERATOR_H