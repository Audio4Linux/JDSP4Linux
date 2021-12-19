#include "DeviceListModel.h"
#include "PresetRuleTableModel.h"

DeviceListModel::DeviceListModel(IAudioService *service, QObject *parent)
    : QAbstractListModel(parent), service(service)
{
}

int DeviceListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return devices.count();
}

QVariant DeviceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto device = devices[index.row()];

    switch (role) {
    case Qt::ItemDataRole::DisplayRole:
        return QString::fromStdString(device.description);
    case Qt::ItemDataRole::UserRole:
        return QVariant::fromValue(device);
    default:
        return QVariant();
    }
}

bool DeviceListModel::loadRemaining(PresetRuleTableModel* ruleModel)
{
    auto devices = service->sinkDevices();
    for(int i = devices.size() - 1; i >= 0; i--)
    {
        if(ruleModel->containsDeviceId(QString::fromStdString(devices.at(i).name)))
        {
            devices.erase(devices.begin() + i);
        }
    }

    if(devices.empty())
    {
        return false;
    }

    load(devices);
    return true;
}

void DeviceListModel::load(const QVector<IOutputDevice> &_devices)
{
    beginResetModel();
    devices = _devices;
    endResetModel();
}

void DeviceListModel::load(const std::vector<IOutputDevice> &_devices)
{
    QVector<IOutputDevice> target;
    for(const auto& device : _devices)
    {
        target.append(device);
    }
    load(target);
}
