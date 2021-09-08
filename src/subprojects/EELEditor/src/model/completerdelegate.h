#ifndef COMPLETERDELEGATE_H
#define COMPLETERDELEGATE_H

#include <QObject>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyle>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QStyleOption>

#include <QDebug>

class CompleterDelegate : public QStyledItemDelegate
{
public:
    CompleterDelegate();

protected:
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

#endif // COMPLETERDELEGATE_H
