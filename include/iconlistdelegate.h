#ifndef ICONLISTDELEGATE_H
#define ICONLISTDELEGATE_H

#include <QStyledItemDelegate>

class IconListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit IconListDelegate(QObject *parent = nullptr);

    void setDarkMode(bool dark);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void drawCardBackground(QPainter *painter, const QRect &rect, bool hovered, bool selected) const;
    bool m_darkMode = false;
};

#endif // ICONLISTDELEGATE_H
