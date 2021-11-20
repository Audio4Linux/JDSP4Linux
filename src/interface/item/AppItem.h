#ifndef APPITEM_H
#define APPITEM_H

#include <AppNode.h>
#include <QWidget>

namespace Ui {
class AppItem;
}

class AppItemModel;

class AppItem : public QWidget
{
    Q_OBJECT

public:
    explicit AppItem(AppItemModel* model, int id, QWidget *parent = nullptr);
    ~AppItem();

private slots:
    void refresh(const AppNode& node);
    void setBlocked(bool blocked);

private:
    Ui::AppItem *ui;
    AppItemModel* model = nullptr;
    uint id;

};

#endif // APPITEM_H
