#ifndef DEVICELISTMODEL_H
#define DEVICELISTMODEL_H

#include <IAudioService.h>
#include <IOutputDevice.h>

#include <QAbstractListModel>
#include <vector>

class PresetRuleTableModel;

class DeviceListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DeviceListModel(IAudioService *service, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool loadRemaining(PresetRuleTableModel* ruleModel);
    void load(const QVector<IOutputDevice>& devices);
    void load(const std::vector<IOutputDevice>& _devices);

private:
    QVector<IOutputDevice> devices;
    IAudioService *service;
};

Q_DECLARE_METATYPE(IOutputDevice)

#endif // DEVICELISTMODEL_H
