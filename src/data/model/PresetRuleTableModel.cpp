#include "PresetRuleTableModel.h"

#include "config/AppConfig.h"
#include "data/PresetManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

PresetRuleTableModel::PresetRuleTableModel(QObject *parent) : QAbstractTableModel(parent)
{

}

int PresetRuleTableModel::rowCount(const QModelIndex &) const { return rules.count(); }

int PresetRuleTableModel::columnCount(const QModelIndex &) const { return 2; }

bool PresetRuleTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows({}, row, row + count - 1);
    rules.remove(row, count);
    endRemoveRows();

    QModelIndex begin = index(row, 0);
    QModelIndex end = index(row, 1);
    emit dataChanged(begin, end);
    return true;
}

bool PresetRuleTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        if (!checkIndex(index))
            return false;
#endif

        auto backup = rules[index.row()];

        switch (index.column()) {
        case 1:
            rules[index.row()].preset = value.toString();
            break;
        };

        /* No change */
        if(backup == rules[index.row()])
            return false;

        emit dataChanged(index, index.sibling(index.row(), 1));
        return true;
    }
    return false;
}

QVariant PresetRuleTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const auto & rule = rules[index.row()];
        switch (index.column()) {
        case 0: return rule.deviceName;
        case 1: return rule.preset;
        default: return {};
        };
    }
    return {};
}

QVariant PresetRuleTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case 0: return tr("Device");
    case 1: return tr("Assigned preset");
    default: return {};
    }
}

Qt::ItemFlags PresetRuleTableModel::flags(const QModelIndex &index) const
{
    switch(index.column())
    {
    case 0:
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    default:
        return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    }
}

void PresetRuleTableModel::load()
{
    beginResetModel();
    rules = PresetManager::instance().rules();
    endResetModel();
}

void PresetRuleTableModel::commit() const
{
    PresetManager::instance().setRules(rules);
}

PresetRule PresetRuleTableModel::at(const QModelIndex &index) const
{
    if(rowCount() < index.row())
        return PresetRule();

    return rules[index.row()];
}

bool PresetRuleTableModel::containsDeviceId(const QString &deviceId) const
{
    for(int i = 0; i < rowCount(); i++)
    {
        if(rules[i].deviceId == deviceId)
        {
            return true;
        }
    }
    return false;
}

bool PresetRuleTableModel::containsDeviceAndRouteId(const QString &deviceId, const QString& routeId) const
{
    for(int i = 0; i < rowCount(); i++)
    {
        if(rules[i].deviceId == deviceId && rules[i].route == routeId)
        {
            return true;
        }
    }
    return false;
}

void PresetRuleTableModel::add(PresetRule rule) {
    beginInsertRows({}, rules.count(), rules.count());
    rules.push_back(rule);
    endInsertRows();

    QModelIndex begin = index(rules.count(), 0);
    QModelIndex end = index(rules.count(), 3);
    emit dataChanged(begin, end);
}

