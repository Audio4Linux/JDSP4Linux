#include "PwAppManager.h"

#include "config/AppConfig.h"

PwAppManager::PwAppManager(PwPipelineManager* mgr) : mgr(mgr)
{
    connect(&AppConfig::instance(), &AppConfig::updated, this, &PwAppManager::handleSettingsUpdate);

    mgr->stream_output_added.connect(sigc::mem_fun(*this, &PwAppManager::onAppAdded));
    mgr->stream_output_changed.connect(sigc::mem_fun(*this, &PwAppManager::onAppChanged));
    mgr->stream_output_removed.connect(sigc::mem_fun(*this, &PwAppManager::onAppRemoved));
}

void PwAppManager::onAppAdded(const uint id, const std::string name, const std::string media_class)
{
    for (int n = 0; n < apps.length(); n++) {
        if (apps[n].id == id) {
            return;
        }
    }

    NodeInfo node_info;

    try {
        node_info = mgr->node_map.at(id);
    } catch (...) {
        return;
    }

    apps.append(node_info);
    emit appAdded(AppNode(node_info));
}

void PwAppManager::onAppChanged(const uint id)
{
    for (int n = 0; n < apps.length(); n++) {
        if (auto item = apps[n]; item.id == id) {
            try {
                item = mgr->node_map.at(id);
            } catch (...) {
                return;
            }

            emit appChanged(AppNode(item));
            break;
        }
    }
}

void PwAppManager::onAppRemoved(const uint id)
{
    for (int n = 0; n < apps.length(); n++)
    {
        if (apps[n].id == id)
        {
            apps.removeAt(n);
            emit appRemoved(id);
            break;
        }
    }
}

QList<AppNode> PwAppManager::activeApps() const
{
    QList<AppNode> appNodes;
    for(const auto& node : apps)
    {
        appNodes.append(node);
    }
    return appNodes;
}

void PwAppManager::handleSettingsUpdate(const AppConfig::Key& key, const QVariant& value)
{
    Q_UNUSED(value)
    switch(key)
    {
    case AppConfig::AudioAppBlocklist:
        for (const auto& node : apps)
        {
            const auto& app_is_enabled = mgr->stream_is_connected(node.id, node.media_class);
            const auto& blocklist = AppConfig::instance().get<QStringList>(AppConfig::AudioAppBlocklist);
            const auto& is_blocklisted = blocklist.contains(QString::fromStdString(node.name));

            if (is_blocklisted && app_is_enabled)
            {
                mgr->disconnect_stream_output(node.id, node.media_class);
            }
            else if (!app_is_enabled)
            {
                mgr->connect_stream_output(node.id, node.media_class);
            }
        }
        break;
    default:
        break;
    }
}
