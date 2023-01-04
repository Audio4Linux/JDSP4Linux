#include "PulsePipelineManager.h"

#include "PulseAppManager.h"

#include "Utils.h"

PulseAppManager::PulseAppManager(PulsePipelineManager* mgr) : mgr(mgr)
{
    connect(&AppConfig::instance(), &AppConfig::updated, this, &PulseAppManager::handleSettingsUpdate);

    mgr->getPulseManager()->sink_input_added.connect(sigc::mem_fun(*this, &PulseAppManager::onAppAdded));
    mgr->getPulseManager()->sink_input_changed.connect(sigc::mem_fun(*this, &PulseAppManager::onAppChanged));
    mgr->getPulseManager()->sink_input_removed.connect(sigc::mem_fun(*this, &PulseAppManager::onAppRemoved));
}

void PulseAppManager::onAppAdded(const std::shared_ptr<AppInfo>& app_info)
{
    for (int n = 0; n < apps.length(); n++) {
        if (apps[n].index == app_info->index) {
            return;
        }
    }

    AppInfo node_info;

    try {
        node_info = *app_info;
    } catch (...) {
        return;
    }

    apps.append(node_info);
    emit appAdded(AppNode(node_info));
}

void PulseAppManager::onAppChanged(const std::shared_ptr<AppInfo>& app_info)
{
    for (int n = 0; n < apps.length(); n++) {
        if (auto item = apps[n]; item.index == app_info->index) {
            try {
                item = *app_info;
            } catch (...) {
                return;
            }

            emit appChanged(AppNode(item));
            break;
        }
    }
}

void PulseAppManager::onAppRemoved(const uint32_t id)
{
    for (int n = 0; n < apps.length(); n++)
    {
        if (apps[n].index == id)
        {
            apps.removeAt(n);
            emit appRemoved(id);
            break;
        }
    }
}

QList<AppNode> PulseAppManager::activeApps() const
{
    QList<AppNode> appNodes;
    for(const auto& node : apps)
    {
        appNodes.append(node);
    }
    return appNodes;
}

void PulseAppManager::handleSettingsUpdate(const AppConfig::Key& key, const QVariant& value)
{
    Q_UNUSED(value)
    switch(key)
    {
    case AppConfig::AudioAppBlocklist: {
        /*std::vector<std::string> blocklist;
        for(const auto& block : AppConfig::instance().get<QStringList>(AppConfig::AudioAppBlocklist))
            blocklist.push_back(block.toStdString());
        mgr->getPulseManager()->blacklist_out = blocklist;

        bool success;
        for(const auto& app : mgr->apps_list)
        {
            for(const auto& block : blocklist)
            {
                if (app.get()->name != block) {
                    if (app->app_type == "sink_input") {
                        success = mgr->getPulseManager()->move_sink_input_to_gstmgr(app->name, app->index);
                    }
                } else {
                    if (app->app_type == "sink_input") {
                        success = mgr->getPulseManager()->remove_sink_input_from_gstmgr(app->name, app->index);
                    }
                }

            }
        }

        if(!success)
        {
            util::error("Blocklist: cannot disable/enable sink");
        }*/
        break;
    }
    default:
        break;
    }
}
