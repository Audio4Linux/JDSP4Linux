#ifndef CLISTVIEW_H
#define CLISTVIEW_H

#include <QListView>
#include <QPainter>

class CListView : public QListView
{
    Q_OBJECT
public:
   CListView(QWidget* parent = 0) : QListView(parent) {}

   bool getEmptyViewEnabled() const;
   void setEmptyViewEnabled(bool newEmptyViewEnabled);

   QString getEmptyViewTitle() const;
   void setEmptyViewTitle(const QString &newEmptyViewTitle);

protected:
   void paintEvent(QPaintEvent *e);

private:
   bool emptyViewEnabled = false;
   QString emptyViewTitle;

};

#endif // CLISTVIEW_H
