#ifndef APPMANAGERFRAGMENT_H
#define APPMANAGERFRAGMENT_H

#include "BaseFragment.h"

class IAppManager;
class AppItemModel;

namespace Ui {
class AppManagerFragment;
}

class AppManagerFragment : public BaseFragment
{
    Q_OBJECT
public:
    explicit AppManagerFragment(IAppManager* appMgr, QWidget *parent = nullptr);
    ~AppManagerFragment();

private slots:
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

private:
    Ui::AppManagerFragment *ui;
    IAppManager* appMgr;
    AppItemModel* model;
};

#endif // APPMANAGERFRAGMENT_H
