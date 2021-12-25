#ifndef VDCDATABASEMODEL_H
#define VDCDATABASEMODEL_H

#include "QJsonTableModel.h"

class VdcDatabaseModel : public QJsonTableModel
{
    Q_OBJECT

public:
    explicit VdcDatabaseModel(QObject *parent = nullptr);

    QString composeVdcFile(int row) const;
    QString coefficients(int row, uint srate) const;
    QString id(int row) const;
    QModelIndex findFirstById(const QString &id) const;
};

#endif // VDCDATABASEMODEL_H
