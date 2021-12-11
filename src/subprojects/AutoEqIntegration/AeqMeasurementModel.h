#ifndef AEQMEASUREMENTMODEL_H
#define AEQMEASUREMENTMODEL_H

#include "AeqStructs.h"

#include <QAbstractListModel>

class AeqMeasurementModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AeqMeasurementModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void import(const QVector<AeqMeasurement> &measurements);
private:
    QVector<AeqMeasurement> items;
};

#endif // AEQMEASUREMENTMODEL_H
