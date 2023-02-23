#ifndef AEQLISTDELEGATES_H
#define AEQLISTDELEGATES_H

#include "AeqMeasurementItem.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <qscopeguard.h>

class AeqItemDelegate :
        public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AeqItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    ~AeqItemDelegate(){}

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex          &index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        auto dummy = new AeqMeasurementItem(AeqMeasurement(), nullptr);
        auto guard = qScopeGuard([dummy] { delete dummy; });
        return QSize(-1, dummy->size().height());
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if(!index.isValid())
        {
            return;
        }

        auto item = index.data(Qt::UserRole).value<AeqMeasurement>();
        QString best;
        switch (item.rank) {
        case -1:
            best = "";
            break;
        case 1:
            best = "";
            break;
        case 2:
            best = tr("2nd choice");
            break;
        case 3:
            best = tr("3rd choice");
            break;
        default:
            best = tr("%1th choice").arg(item.rank);
            break;
        }
        auto bestWidth = painter->fontMetrics().horizontalAdvance(best);

        QRect displayRect = QRect(option.rect.x() + 18, option.rect.y() + 10,
                                  option.rect.width() - 18, (option.rect.height() - 4) / 2);

        QRect displayRect2 = QRect(option.rect.x() + 18, option.rect.y() + option.rect.height() / 2,
                                   option.rect.width() - 18 - bestWidth, (option.rect.height() - 8) / 2);

        QRect bestIndicator = QRect(option.rect.width() - 18 - bestWidth, option.rect.y() + option.rect.height() / 2,
                                   option.rect.width() - 18, (option.rect.height() - 26) / 2);

        QStyleOptionViewItem newOption(option);

        QPalette::ColorRole textRole;
        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
            textRole = QPalette::ColorRole::HighlightedText;
        }
        else
        {
            textRole = QPalette::ColorRole::Text;
        }

        painter->setPen(option.palette.color(textRole));

        QFont bold(painter->font());
        QFont normal(painter->font());
        QFont small(painter->font());
        bold.setBold(true);
        small.setPointSize(8);

        painter->setFont(bold);
        painter->drawText(displayRect, item.name);
        painter->setFont(normal);
        painter->drawText(displayRect2, item.source);

        painter->setFont(small);
        painter->setPen(option.palette.color(QPalette::ColorGroup::Disabled, textRole));
        painter->drawText(bestIndicator, best);

        painter->setFont(normal);
    }

};

#endif // AEQLISTDELEGATES_H
