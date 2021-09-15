#ifndef PWAPPMANAGER_H
#define PWAPPMANAGER_H

#include "PwPipelineManager.h"
#include "IAppManager.h"

#include <QObject>
#include <memory>

class PwAppManager : public IAppManager
{
    Q_OBJECT
public:
    explicit PwAppManager(PwPipelineManager* mgr);

signals:
    void appAdded(const NodeInfo& node);
    void appChanged(const NodeInfo& node);
    void appRemoved(const uint id);

private:
    void onAppAdded(const uint id, const std::string name, const std::string media_class);
    void onAppChanged(const uint id);
    void onAppRemoved(const uint id);

    QList<NodeInfo> apps;

    PwPipelineManager* mgr;
};

#endif // PWAPPMANAGER_H
