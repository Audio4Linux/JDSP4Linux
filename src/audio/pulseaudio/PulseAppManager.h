#ifndef PULSEAPPMANAGER_H
#define PULSEAPPMANAGER_H

#include <IAppManager.h>
#include <config/AppConfig.h>

class PulsePipelineManager;

class PulseAppManager : public IAppManager
{
public:
    explicit PulseAppManager(PulsePipelineManager* mgr);

    QList<AppNode> activeApps() const override;

private slots:
    void handleSettingsUpdate(const AppConfig::Key&, const QVariant&);

private:
    void onAppAdded(const std::shared_ptr<AppInfo> &app_info);
    void onAppChanged(const std::shared_ptr<AppInfo> &app_info);
    void onAppRemoved(const uint id);

    QList<AppInfo> apps;
    PulsePipelineManager* mgr;

};

#endif // PULSEAPPMANAGER_H
