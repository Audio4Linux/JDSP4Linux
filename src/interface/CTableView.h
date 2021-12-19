#ifndef CTABLEVIEW_H
#define CTABLEVIEW_H

#include <QTableView>
#include <QPainter>

class CTableView : public QTableView
{
    Q_OBJECT
public:
   CTableView(QWidget* parent = 0) : QTableView(parent) {}

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

#endif // CTABLEVIEW_H
