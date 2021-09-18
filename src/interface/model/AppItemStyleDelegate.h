#ifndef APPITEMSTYLEDELEGATE_H
#define APPITEMSTYLEDELEGATE_H

#include <QStyledItemDelegate>

class AppItemStyleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        return QSize(-1, 78);
    }
};

#endif // APPITEMSTYLEDELEGATE_H
