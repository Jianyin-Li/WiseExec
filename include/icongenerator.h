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
    
    static QIcon generateDefaultIcon(const QString &name, int size = 64);

    static QIcon generateIcon(const QString &text, const QColor &backgroundColor,
                             const QColor &textColor = Qt::white, int size = 64);

    static const QList<QColor>& getDefaultColors();

    static QColor getColorForName(const QString &name);

private:
    static QPixmap drawCircularIcon(const QString &text, const QColor &backgroundColor,
                                   const QColor &textColor, int size);

    static QPixmap drawSquareIcon(const QString &text, const QColor &backgroundColor,
                                 const QColor &textColor, int size);
};

#endif // ICONGENERATOR_H