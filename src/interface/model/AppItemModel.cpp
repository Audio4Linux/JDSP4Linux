#include "AppItemModel.h"

AppItemModel::AppItemModel(IAppManager* appMgr, QObject *parent)
    : QAbstractListModel(parent), appMgr(appMgr)
{
    assert(appMgr != nullptr);
    cache = appMgr->activeApps();

    connect(appMgr, &IAppManager::appChanged, this, &AppItemModel::appChanged);

    connect(appMgr, &IAppManager::appAdded,   this, &AppItemModel::onAppAdded);
    connect(appMgr, &IAppManager::appChanged, this, &AppItemModel::onAppChanged);
    connect(appMgr, &IAppManager::appRemoved, this, &AppItemModel::onAppRemoved);
}

QModelIndex AppItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column);
}

int AppItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    assert(appMgr != nullptr);
    return cache.count();
}

QVariant AppItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role == Qt::ItemDataRole::UserRole)
        return QVariant::fromValue(cache.at(index.row()));
    else
        return QVariant();
}

std::optional<AppNode> AppItemModel::findByNodeId(uint id)
{
    for(int i = 0; i < cache.count(); i++)
    {
        if(cache.at(i).id == id)
        {
            return cache.at(i);
        }
    }
    return std::nullopt;
}

void AppItemModel::onAppAdded(const AppNode &node)
{
    // Remove node if id already exists
    onAppRemoved(node.id);

    beginInsertRows(QModelIndex(), cache.count(), cache.count());
    cache.append(node);
    endInsertRows();
}

void AppItemModel::onAppChanged(const AppNode &node)
{
    for(int i = 0; i < cache.count(); i++)
    {
        if(cache.at(i).id == node.id)
        {
            cache[i] = node;
            emit dataChanged(index(i, 0), index(i, 0));
            break;
        }
    }
}

void AppItemModel::onAppRemoved(const uint id)
{
    for(int i = 0; i < cache.count(); i++)
    {
        if(cache.at(i).id == id)
        {
            beginRemoveRows(QModelIndex(), i, i);
            cache.removeAt(i);
            endRemoveRows();
            break;
        }
    }
}
