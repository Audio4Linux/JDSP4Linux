#ifndef DELEGATES_H
#define DELEGATES_H
#include <QStyledItemDelegate>
class ItemSizeDelegate : public QStyledItemDelegate  {
public:
    ItemSizeDelegate(QObject *parent=0) : QStyledItemDelegate (parent){}

    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const{
        return QSize(option.widget->height(), 55);
    }
};
#endif // DELEGATES_H
