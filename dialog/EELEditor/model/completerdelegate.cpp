#include "completerdelegate.h"

#include <QToolTip>

CompleterDelegate::CompleterDelegate()
{

}

void CompleterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem optionV4 = option;
    initStyleOption(&optionV4, index);
    QStyleOptionViewItem hint = option;
    initStyleOption(&hint, index.sibling(index.row(),1));

    QStyle *style = optionV4.widget? optionV4.widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setHtml(optionV4.text+" <b>"+hint.text+"</b>");

    // Painting item without text
    optionV4.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
    painter->save();
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));

    // Highlighting text if item is selected
    if (optionV4.state & QStyle::State_Selected){
        ctx.palette.setColor(QPalette::Text, optionV4.palette.color(QPalette::Active, QPalette::HighlightedText));
        QStyleOptionViewItem toolTip = option;
        initStyleOption(&toolTip, index.sibling(index.row(),2));
        QToolTip::showText(QPoint(textRect.right() + 15, textRect.top()), toolTip.text );
    }

    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize CompleterDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem optionV4 = option;
    initStyleOption(&optionV4, index);
    QStyleOptionViewItem hint = option;
    initStyleOption(&hint, index.sibling(index.row(),1));

    QTextDocument doc;
    doc.setHtml(optionV4.text+" <b>"+hint.text+"</b>");
    doc.setTextWidth(optionV4.rect.width());
    return QSize(doc.idealWidth(), doc.size().height());
}
