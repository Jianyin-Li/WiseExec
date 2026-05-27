#include "icongenerator.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QByteArray>

IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent)
{
}

QIcon IconGenerator::generateDefaultIcon(const QString &name, int size)
{
    if (name.isEmpty()) {
        return generateIcon("?", Qt::gray, Qt::white, size);
    }
    
    // 获取名称的第一个字符（大写）
    QString firstChar = name.left(1).toUpper();
    
    // 根据名称获取颜色
    QColor backgroundColor = getColorForName(name);
    
    return generateIcon(firstChar, backgroundColor, Qt::white, size);
}

QIcon IconGenerator::generateIcon(const QString &text, const QColor &backgroundColor,
                                 const QColor &textColor, int size)
{
    // 生成圆形图标
    QPixmap pixmap = drawCircularIcon(text, backgroundColor, textColor, size);
    return QIcon(pixmap);
}

const QList<QColor>& IconGenerator::getDefaultColors()
{
    static const QList<QColor> colors = {
        QColor(66, 133, 244),   // 蓝色
        QColor(219, 68, 55),    // 红色
        QColor(244, 180, 0),    // 黄色
        QColor(15, 157, 88),    // 绿色
        QColor(171, 71, 188),   // 紫色
        QColor(0, 172, 193),    // 青色
        QColor(255, 112, 67),   // 橙色
        QColor(121, 85, 72),    // 棕色
        QColor(158, 158, 158),  // 灰色
        QColor(96, 125, 139)    // 蓝灰色
    };
    return colors;
}

QColor IconGenerator::getColorForName(const QString &name)
{
    if (name.isEmpty()) {
        return Qt::gray;
    }
    
    // 使用哈希函数从名称生成确定性颜色
    QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5);
    
    // 从哈希值获取颜色索引
    int colorIndex = static_cast<unsigned char>(hash[0]) % getDefaultColors().size();
    
    return getDefaultColors().at(colorIndex);
}

QPixmap IconGenerator::drawCircularIcon(const QString &text, const QColor &backgroundColor,
                                       const QColor &textColor, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // 绘制圆形背景
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, size - 4, size - 4);
    
    // 绘制文字
    painter.setPen(textColor);
    
    // 根据图标大小调整字体大小
    int fontSize = size * 0.5;
    if (text.length() > 1) {
        fontSize = size * 0.35;
    }
    
    QFont font;
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);
    
    // 居中绘制文字
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(text);
    
    int x = (size - textRect.width()) / 2 - textRect.left();
    int y = (size - textRect.height()) / 2 - textRect.top();
    
    painter.drawText(x, y, text);
    
    return pixmap;
}

QPixmap IconGenerator::drawSquareIcon(const QString &text, const QColor &backgroundColor,
                                     const QColor &textColor, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // 绘制圆角矩形背景
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(2, 2, size - 4, size - 4, 10, 10);
    
    // 绘制文字
    painter.setPen(textColor);
    
    // 根据图标大小调整字体大小
    int fontSize = size * 0.5;
    if (text.length() > 1) {
        fontSize = size * 0.35;
    }
    
    QFont font;
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);
    
    // 居中绘制文字
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(text);
    
    int x = (size - textRect.width()) / 2 - textRect.left();
    int y = (size - textRect.height()) / 2 - textRect.top();
    
    painter.drawText(x, y, text);
    
    return pixmap;
}