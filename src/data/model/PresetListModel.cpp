#include "PresetListModel.h"

#include "config/AppConfig.h"

#include <QDir>

PresetListModel::PresetListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    rescan();
}

int PresetListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return presets.count();
}

QVariant PresetListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch(role)
    {
    case Qt::DisplayRole:
        return presets.at(index.row());
    case Qt::UserRole:
        return presets.at(index.row());
    default:
        return QVariant();
    }
}

void PresetListModel::rescan()
{
    beginResetModel();
    presets.clear();

    QDir dir(AppConfig::instance().getPath("presets"));
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    QStringList nameFilter("*.conf");
    QStringList files = dir.entryList(nameFilter);

    for (int i = 0; i < files.count(); i++)
    {
        presets.append(QFileInfo(files[i]).completeBaseName());
    }

    endResetModel();
}

QStringList PresetListModel::getList() const
{
    return presets;
}
