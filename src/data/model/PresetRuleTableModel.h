#ifndef PRESETRULETABLEMODEL_H
#define PRESETRULETABLEMODEL_H

#include <QAbstractTableModel>

#include "data/PresetRule.h"

class PresetRuleTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PresetRuleTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool containsDeviceId(const QString& deviceId) const;
    bool containsDeviceAndRouteId(const QString &deviceId, const QString& routeId) const;

    PresetRule at(const QModelIndex &index) const;
    void add(PresetRule rule);

public slots:
    void load();
    void commit() const;

private:
    QVector<PresetRule> rules;

};

#endif // PRESETRULETABLEMODEL_H
