#include "CListView.h"

void CListView::paintEvent(QPaintEvent *e) {
    QListView::paintEvent(e);

    if (!emptyViewEnabled || (model() && model()->rowCount(rootIndex()) > 0))
    {
        return;
    }

    QPainter p(this->viewport());
    p.setPen(this->palette().placeholderText().color());
    p.drawText(rect(), Qt::AlignCenter, emptyViewTitle);
}

bool CListView::getEmptyViewEnabled() const
{
    return emptyViewEnabled;
}

void CListView::setEmptyViewEnabled(bool newEmptyViewEnabled)
{
    emptyViewEnabled = newEmptyViewEnabled;
}

QString CListView::getEmptyViewTitle() const
{
    return emptyViewTitle;
}

void CListView::setEmptyViewTitle(const QString &newEmptyViewTitle)
{
    emptyViewTitle = newEmptyViewTitle;
}
