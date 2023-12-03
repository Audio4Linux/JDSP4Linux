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
    explicit RouteListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool loadRemaining(IOutputDevice device, PresetRuleTableModel* ruleModel);
    void load(const QVector<Route>& routes);

    static Route makeDefaultRoute();

private:
    QVector<Route> routes;
};

#endif // ROUTELISTMODEL_H
