#include "CTableView.h"

void CTableView::paintEvent(QPaintEvent *e) {
    QTableView::paintEvent(e);

    if (!emptyViewEnabled || (model() && model()->rowCount(rootIndex()) > 0))
    {
        return;
    }

    QPainter p(this->viewport());
    p.setPen(this->palette().placeholderText().color());
    p.drawText(rect(), Qt::AlignCenter, emptyViewTitle);
}

bool CTableView::getEmptyViewEnabled() const
{
    return emptyViewEnabled;
}

void CTableView::setEmptyViewEnabled(bool newEmptyViewEnabled)
{
    emptyViewEnabled = newEmptyViewEnabled;
}

QString CTableView::getEmptyViewTitle() const
{
    return emptyViewTitle;
}

void CTableView::setEmptyViewTitle(const QString &newEmptyViewTitle)
{
    emptyViewTitle = newEmptyViewTitle;
}
