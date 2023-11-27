#include "PwAppManager.h"

#include "config/AppConfig.h"

PwAppManager::PwAppManager(PwPipelineManager* mgr) : mgr(mgr)
{
    connect(&AppConfig::instance(), &AppConfig::updated, this, &PwAppManager::handleSettingsUpdate);

    mgr->stream_output_added.connect(sigc::mem_fun(*this, &PwAppManager::onAppAdded));
    mgr->stream_output_changed.connect(sigc::mem_fun(*this, &PwAppManager::onAppChanged));
    mgr->stream_output_removed.connect(sigc::mem_fun(*this, &PwAppManager::onAppRemoved));
}

void PwAppManager::onAppAdded(const NodeInfo& node)
{
    for (int n = 0; n < apps.length(); n++) {
        if (apps[n].serial == node.serial) {
            return;
        }
    }

    NodeInfo node_info;

    try {
        node_info = mgr->node_map.at(node.serial);
    } catch (...) {
        return;
    }

    apps.append(node_info);
    emit appAdded(AppNode(node_info));
}

void PwAppManager::onAppChanged(const NodeInfo& node)
{
    for (int n = 0; n < apps.length(); n++) {
        if (auto item = apps[n]; item.serial == node.serial) {
            try {
                item = mgr->node_map.at(node.serial);
            } catch (...) {
                return;
            }

            emit appChanged(AppNode(item));
            break;
        }
    }
}

void PwAppManager::onAppRemoved(const uint64_t serial)
{
    for (int n = 0; n < apps.length(); n++)
    {
        if (apps[n].serial == serial)
        {
            apps.removeAt(n);
            emit appRemoved(serial);
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
        for (const auto& node : std::as_const(apps))
        {
            const auto& app_is_enabled = mgr->stream_is_connected(node.id, node.media_class);
            bool is_blocklisted = AppConfig::instance().isAppBlocked(QString::fromStdString(node.name));

            if (is_blocklisted && app_is_enabled)
            {
                mgr->disconnect_stream(node.id);
            }
            else if (!app_is_enabled)
            {
                if (node.media_class == tags::pipewire::media_class::output_stream) {
                  mgr->connect_stream_output(node.id);
                }
            }
        }
        break;
    default:
        break;
    }
}
