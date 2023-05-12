#ifndef PRESETLISTMODEL_H
#define PRESETLISTMODEL_H

#include <QAbstractListModel>

class PresetListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PresetListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void rescan();
    QStringList getList() const;

private:

    QStringList presets;
};

#endif // PRESETLISTMODEL_H
