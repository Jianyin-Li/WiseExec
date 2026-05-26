#include "iconlistdelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

static const int CARD_WIDTH = 110;
static const int CARD_HEIGHT = 120;
static const int CARD_RADIUS = 12;
static const int ICON_SIZE = 56;
static const int ICON_Y_OFFSET = 12;
static const int TEXT_Y_OFFSET = 76;
static const int TEXT_HEIGHT = 36;

IconListDelegate::IconListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize IconListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(CARD_WIDTH, CARD_HEIGHT);
}

void IconListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    bool hovered = option.state & QStyle::State_MouseOver;
    bool selected = option.state & QStyle::State_Selected;

    QRect cardRect = option.rect.adjusted(4, 4, -4, -4);

    drawCardBackground(painter, cardRect, hovered, selected);

    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        int iconX = cardRect.center().x() - ICON_SIZE / 2;
        int iconY = cardRect.top() + ICON_Y_OFFSET;
        QPixmap pixmap = icon.pixmap(ICON_SIZE, ICON_SIZE);

        QPainterPath clipPath;
        clipPath.addEllipse(iconX, iconY, ICON_SIZE, ICON_SIZE);
        painter->setClipPath(clipPath);
        painter->drawPixmap(iconX, iconY, ICON_SIZE, ICON_SIZE, pixmap);
        painter->setClipping(false);
    }

    QString text = index.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        QRect textRect(
            cardRect.left() + 4,
            cardRect.top() + TEXT_Y_OFFSET,
            cardRect.width() - 8,
            TEXT_HEIGHT
        );

        QFont font = painter->font();
        font.setPointSize(8);
        font.setBold(false);
        painter->setFont(font);

        QColor textColor = selected ? QColor("#1a73e8") : QColor("#3c4043");
        painter->setPen(textColor);

        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
}

void IconListDelegate::drawCardBackground(QPainter *painter, const QRect &rect, bool hovered, bool selected) const
{
    QColor bgColor = selected ? QColor("#e8f0fe") : QColor("#ffffff");
    QColor borderColor;
    qreal borderWidth;

    if (selected) {
        borderColor = QColor("#1a73e8");
        borderWidth = 1.5;
    } else if (hovered) {
        borderColor = QColor("#dadce0");
        borderWidth = 1.0;
    } else {
        borderColor = QColor("#f0f0f0");
        borderWidth = 0.0;
    }

    if (hovered && !selected) {
        bgColor = QColor("#f8f9fa");
    }

    if (borderWidth > 0) {
        QPainterPath shadowPath;
        shadowPath.addRoundedRect(rect.adjusted(0, 2, 0, 0), CARD_RADIUS, CARD_RADIUS);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 15));
        painter->drawPath(shadowPath);
    }

    QPainterPath cardPath;
    cardPath.addRoundedRect(rect, CARD_RADIUS, CARD_RADIUS);
    painter->setPen(QPen(borderColor, borderWidth));
    painter->setBrush(bgColor);
    painter->drawPath(cardPath);
}
