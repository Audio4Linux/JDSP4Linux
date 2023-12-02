#ifndef ROUTELISTMODEL_H
#define ROUTELISTMODEL_H

#include <IOutputDevice.h>

#include <QAbstractListModel>
#include <vector>

class PresetRuleTableModel;

class RouteListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit RouteListModel(IOutputDevice device, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool loadRemaining(PresetRuleTableModel* ruleModel);
    void load(const QVector<Route>& routes);
    void load(const std::vector<Route>& routes);

private:
    QVector<Route> routes;
    IOutputDevice device;
};

#endif // ROUTELISTMODEL_H
