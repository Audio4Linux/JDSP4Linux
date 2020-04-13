#ifndef CONFIGITEM_H
#define CONFIGITEM_H

#include <QWidget>

namespace Ui {
class configitem;
}

class DetailListItem : public QWidget
{
    Q_OBJECT

public:
    explicit DetailListItem(QWidget *parent = nullptr);
    ~DetailListItem();
    void setData(QString title, QString desc);

private:
    Ui::configitem *ui;
};

#endif // CONFIGITEM_H
