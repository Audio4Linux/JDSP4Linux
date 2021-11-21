#include "VdcDatabaseModel.h"

#include <QTextStream>
#include <QFile>

VdcDatabaseModel::VdcDatabaseModel(QObject *parent)
    : QJsonTableModel(QJsonTableModel::Header(), parent)
{
    QJsonTableModel::Header header;
    header.push_back(QJsonTableModel::Heading({
        { "title", "Company" },   { "index", "Company" }
    }));
    header.push_back(QJsonTableModel::Heading({
        { "title", "Model" }, { "index", "Model" }
    }));
    header.push_back(QJsonTableModel::Heading({
        { "title", "SR_44100_Coeffs" }, { "index", "SR_44100_Coeffs" }
    }));
    header.push_back(QJsonTableModel::Heading({
        { "title", "SR_48000_Coeffs" }, { "index", "SR_48000_Coeffs" }
    }));
    header.push_back(QJsonTableModel::Heading({
        { "title", "ID" }, { "index", "ID" }
    }));
    this->setHeader(header);

    QFile file(":/assets/DDCData.json");

    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream   instream(&file);
        QJsonDocument jsonDocument = QJsonDocument::fromJson(instream.readAll().toLocal8Bit());
        this->setJson(jsonDocument);
    }

    this->setHeaderData(0, Qt::Horizontal, tr("Company"));
    this->setHeaderData(1, Qt::Horizontal, tr("Model"));
}


QString VdcDatabaseModel::coefficients(int row, uint srate) const
{
    Q_ASSERT(srate == 44100 || srate == 48000);

    if(srate == 44100)
    {
        return this->data(this->index(row, 2), Qt::DisplayRole).toString();
    }

    return this->data(this->index(row, 3), Qt::DisplayRole).toString();
}

QString VdcDatabaseModel::id(int row) const
{
    return this->data(this->index(row, 4), Qt::DisplayRole).toString();
}

QModelIndex VdcDatabaseModel::findFirstById(const QString& id) const
{
    auto matches = this->match(this->index(0, 4), Qt::DisplayRole, id, 1);

    foreach(const QModelIndex &index, matches)
    {
        if (id == this->data(this->index(index.row(), 4), Qt::DisplayRole))
        {
            return index;
        }
    }

    return QModelIndex();
}
