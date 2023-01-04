#ifndef APPITEMMODEL_H
#define APPITEMMODEL_H

#include <IAppManager.h>
#include <QAbstractItemModel>

#include <optional>

class AppItemModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AppItemModel(IAppManager* appMgr, QObject *parent = nullptr);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    std::optional<AppNode> findByNodeId(uint32_t id);

signals:
    void appChanged(const AppNode& node);

private slots:
    void onAppAdded(const AppNode& node);
    void onAppChanged(const AppNode& node);
    void onAppRemoved(const uint32_t id);

private:
    IAppManager* appMgr = nullptr;

    QList<AppNode> cache;
};

#endif // APPITEMMODEL_H
