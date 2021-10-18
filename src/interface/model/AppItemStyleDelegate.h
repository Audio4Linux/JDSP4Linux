#ifndef APPITEMSTYLEDELEGATE_H
#define APPITEMSTYLEDELEGATE_H

#include "AppItem.h"

#include <QStyledItemDelegate>
#include <QScopeGuard>

class AppItemStyleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        auto dummy = new AppItem(nullptr, -1);
        auto guard = qScopeGuard([dummy] { delete dummy; });
        return QSize(-1, dummy->size().height());
    }
};

#endif // APPITEMSTYLEDELEGATE_H
