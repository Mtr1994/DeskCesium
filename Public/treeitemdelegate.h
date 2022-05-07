#ifndef TREEITEMDELEGATE_H
#define TREEITEMDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPainter>
#include <QRect>

class TreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TreeItemDelegate(){}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        painter->setPen(Qt::NoPen);
        painter->setRenderHint(QPainter::Antialiasing);

        QRect rect(option.rect);
        rect.setLeft(0);
        if (option.state & QStyle::State_Selected )
        {
            painter->setBrush(QBrush("#557cb3f1"));
            painter->drawRoundedRect(rect, 3, 3);
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            painter->setBrush(QBrush("#337cb3f1"));
            painter->drawRoundedRect(rect, 3, 3);
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
};

#endif // TREEITEMDELEGATE_H
