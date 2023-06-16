#ifndef PWAPPMANAGER_H
#define PWAPPMANAGER_H

#include "PwPipelineManager.h"
#include "IAppManager.h"
#include "config/AppConfig.h"

#include "AppNode.h"
#include <QObject>
#include <memory>

class PwAppManager : public IAppManager
{
    Q_OBJECT
public:
    explicit PwAppManager(PwPipelineManager* mgr);

    QList<AppNode> activeApps() const override;

private slots:
    void handleSettingsUpdate(const AppConfig::Key&, const QVariant&);

private:
    void onAppAdded(const NodeInfo& serial);
    void onAppChanged(const NodeInfo& serial);
    void onAppRemoved(const uint64_t serial);

    QList<NodeInfo> apps;

    PwPipelineManager* mgr;
};

#endif // PWAPPMANAGER_H
