#ifndef APPMANAGERFRAGMENT_H
#define APPMANAGERFRAGMENT_H

#include <QWidget>

class IAppManager;
class AppItemModel;

namespace Ui {
class AppManagerFragment;
}

class AppManagerFragment : public QWidget
{
    Q_OBJECT

public:
    explicit AppManagerFragment(IAppManager* appMgr, QWidget *parent = nullptr);
    ~AppManagerFragment();

signals:
    void closePressed();

private slots:
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

private:
    Ui::AppManagerFragment *ui;
    IAppManager* appMgr;
    AppItemModel* model;
};

#endif // APPMANAGERFRAGMENT_H
