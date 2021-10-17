#include "interface/model/AppItemModel.h"

#include "AppManagerFragment.h"
#include "ui_AppManagerFragment.h"

#include "interface/model/AppItem.h"
#include "interface/model/AppItemStyleDelegate.h"

AppManagerFragment::AppManagerFragment(IAppManager* appMgr, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppManagerFragment),
    appMgr(appMgr)
{
    ui->setupUi(this);

    model = new AppItemModel(appMgr, this);

    ui->apps->setEmptyViewEnabled(true);
    ui->apps->setEmptyViewTitle("No apps are playing audio");
    ui->apps->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    ui->apps->setModel(model);
    ui->apps->setItemDelegate(new AppItemStyleDelegate(ui->apps));

    connect(model, &AppItemModel::rowsInserted, this, &AppManagerFragment::rowsInserted);
    connect(model, &AppItemModel::rowsAboutToBeRemoved, this, &AppManagerFragment::rowsAboutToBeRemoved);

    connect(ui->close, &QPushButton::clicked, this, &AppManagerFragment::closePressed);
}

AppManagerFragment::~AppManagerFragment()
{
    disconnect(model, &AppItemModel::rowsInserted, this, &AppManagerFragment::rowsInserted);
    disconnect(model, &AppItemModel::rowsAboutToBeRemoved, this, &AppManagerFragment::rowsAboutToBeRemoved);
    delete ui;
    delete model;
}

void AppManagerFragment::rowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    auto node = qvariant_cast<AppNode>(model->index(first, 0).data(Qt::UserRole));

    ui->apps->setIndexWidget(model->index(first, 0), new AppItem(model, node.id, this));
}

void AppManagerFragment::rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    auto* widget = ui->apps->indexWidget(model->index(first, 0));
    if(widget != nullptr)
    {
        widget->deleteLater();
    }
    ui->apps->setIndexWidget(model->index(first, 0), nullptr);
}
