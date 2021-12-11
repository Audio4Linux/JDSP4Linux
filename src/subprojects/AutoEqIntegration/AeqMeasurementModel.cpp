#include "AeqMeasurementModel.h"

AeqMeasurementModel::AeqMeasurementModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void AeqMeasurementModel::import(const QVector<AeqMeasurement>& measurements)
{
    beginResetModel();
    items = measurements;
    endResetModel();
}

int AeqMeasurementModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return items.size();
}

QVariant AeqMeasurementModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role == Qt::ItemDataRole::UserRole)
        return QVariant::fromValue(items.at(index.row()));
    else if (role == Qt::ItemDataRole::DisplayRole)
        return items.at(index.row()).name;
    else
        return QVariant();
}
