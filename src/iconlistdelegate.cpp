#include "iconlistdelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

static const int CARD_WIDTH = 120;
static const int CARD_HEIGHT = 140;
static const int CARD_RADIUS = 14;
static const int ICON_SIZE = 56;
static const int ICON_Y_OFFSET = 14;
static const int TEXT_Y_OFFSET = 80;
static const int TEXT_HEIGHT = 42;

IconListDelegate::IconListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void IconListDelegate::setDarkMode(bool dark)
{
    m_darkMode = dark;
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

    QRect cardRect = option.rect.adjusted(5, 5, -5, -5);

    drawCardBackground(painter, cardRect, hovered, selected);

    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        int iconX = cardRect.center().x() - ICON_SIZE / 2;
        int iconY = cardRect.top() + ICON_Y_OFFSET;
        QPixmap pixmap = icon.pixmap(ICON_SIZE, ICON_SIZE);

        painter->save();
        QPainterPath clipPath;
        clipPath.addEllipse(iconX + 1, iconY + 1, ICON_SIZE - 2, ICON_SIZE - 2);
        painter->setClipPath(clipPath);
        painter->drawPixmap(iconX + 1, iconY + 1, ICON_SIZE - 2, ICON_SIZE - 2, pixmap);
        painter->restore();
    }

    QString text = index.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        QRect textRect(
            cardRect.left() + 6,
            cardRect.top() + TEXT_Y_OFFSET,
            cardRect.width() - 12,
            TEXT_HEIGHT
        );

        QFont font = painter->font();
        font.setPixelSize(11);
        font.setBold(false);
        painter->setFont(font);

        QColor textColor;
        if (m_darkMode) {
            textColor = selected ? QColor("#8ab4f8") : QColor("#e8eaed");
        } else {
            textColor = selected ? QColor("#1a73e8") : QColor("#3c4043");
        }
        painter->setPen(textColor);

        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
}

void IconListDelegate::drawCardBackground(QPainter *painter, const QRect &rect, bool hovered, bool selected) const
{
    QColor bgColor, borderColor;
    qreal borderWidth;
    int shadowOffset = hovered ? 4 : 2;

    if (m_darkMode) {
        bgColor = selected ? QColor("#2d3a4a") : QColor("#2d2d2d");
        borderColor = selected ? QColor("#8ab4f8") : (hovered ? QColor("#3c4043") : QColor("#333333"));
        borderWidth = selected ? 1.5 : (hovered ? 1.0 : 0.0);
    } else {
        bgColor = selected ? QColor("#e8f0fe") : QColor("#ffffff");
        borderColor = selected ? QColor("#1a73e8") : (hovered ? QColor("#dadce0") : QColor("#f0f0f0"));
        borderWidth = selected ? 1.5 : (hovered ? 1.0 : 0.0);
    }

    if (hovered && !selected) {
        bgColor = m_darkMode ? QColor("#333333") : QColor("#f8f9fa");
    }

    QColor shadowColor = m_darkMode ? QColor(0, 0, 0, 60) : QColor(0, 0, 0, 20);
    if (hovered) {
        shadowColor = m_darkMode ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 35);
    }

    for (int i = 0; i < 3; ++i) {
        QPainterPath shadowPath;
        int blur = i * 2;
        shadowPath.addRoundedRect(rect.adjusted(-blur, shadowOffset + blur, blur, blur), CARD_RADIUS, CARD_RADIUS);
        painter->setPen(Qt::NoPen);
        QColor blurColor = shadowColor;
        blurColor.setAlpha(shadowColor.alpha() / (i + 2));
        painter->setBrush(blurColor);
        painter->drawPath(shadowPath);
    }

    QPainterPath cardPath;
    cardPath.addRoundedRect(rect, CARD_RADIUS, CARD_RADIUS);
    if (borderWidth > 0) {
        painter->setPen(QPen(borderColor, borderWidth));
    } else {
        painter->setPen(Qt::NoPen);
    }
    painter->setBrush(bgColor);
    painter->drawPath(cardPath);
}
