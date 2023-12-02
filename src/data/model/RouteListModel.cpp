#include "RouteListModel.h"
#include "PresetRuleTableModel.h"

RouteListModel::RouteListModel(IOutputDevice device, QObject *parent)
    : QAbstractListModel(parent), device(device)
{
}

int RouteListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return routes.count();
}

QVariant RouteListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto route = routes[index.row()];

    switch (role) {
    case Qt::ItemDataRole::DisplayRole:
        return QString::fromStdString(route.description);
    case Qt::ItemDataRole::UserRole:
        return QVariant::fromValue(route);
    default:
        return QVariant();
    }
}

bool RouteListModel::loadRemaining(PresetRuleTableModel* ruleModel)
{
    auto routes = device.output_routes.toVector();
    for(int i = routes.size() - 1; i >= 0; i--)
    {
        if(ruleModel->containsDeviceAndRouteId(QString::fromStdString(device.name), QString::fromStdString(routes.at(i).name)))
        {
            routes.erase(routes.begin() + i);
        }
    }

    if(routes.empty())
    {
        return false;
    }

    load(routes);
    return true;
}

void RouteListModel::load(const QVector<Route> &_routes)
{
    beginResetModel();
    routes = _routes;
    endResetModel();
}

void RouteListModel::load(const std::vector<Route> &_routes)
{
    QVector<Route> target;
    for(const auto& device : _routes)
    {
        target.append(device);
    }
    load(target);
}
