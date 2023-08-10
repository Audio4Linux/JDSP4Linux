#include <glibmm.h>

#include "PipewireAudioService.h"
#include "PwJamesDspPlugin.h"

#include "PwPipelineManager.h"
#include "DspHost.h"
#include "PwDevice.h"
#include "Utils.h"

#include <QDebug>

PipewireAudioService::PipewireAudioService()
{
    Glib::init();

    mgr = std::make_unique<PwPipelineManager>(AppConfig::instance().get<bool>(AppConfig::AudioVirtualSinkForceMaxValue));
    appMgr = std::make_unique<PwAppManager>(mgr.get());
    plugin = new PwJamesDspPlugin(mgr.get(), this);
    effects = std::make_unique<FilterContainer>(mgr.get(), plugin, &AppConfig::instance());

    mgr.get()->new_default_sink_name.connect([=](const std::string& name) {
        util::debug("new default output device: " + name);

        if (AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault)) {
            /*
             *  Depending on the hardware headphones can cause a node recreation where the id and the name are kept.
             *  So we clear the key to force the callbacks to be called
             */

            AppConfig::instance().set(AppConfig::AudioOutputDevice, "");
            AppConfig::instance().set(AppConfig::AudioOutputDevice, QString::fromStdString(name));
        }
    });

    mgr.get()->device_output_route_changed.connect([this](DeviceInfo device) {
        if (device.output_route_available == SPA_PARAM_AVAILABILITY_no)
        {
            return;
        }

        util::debug("device "s + device.name + " has changed its output route to: "s + device.output_route_name);


        NodeInfo target_node;
        for (const auto& [serial, node] : mgr.get()->node_map)
        {
            if (node.media_class == tags::pipewire::media_class::sink) {
              if (util::str_contains(node.name, device.bus_path) || util::str_contains(node.name, device.bus_id)) {
                target_node = node;

                break;
              }
            }
        }

        if (target_node.serial != SPA_ID_INVALID)
        {
            emit outputDeviceChanged(QString::fromStdString(target_node.description), QString::fromStdString(device.name));
        }
        else
        {
            util::debug("device_output_route_changed: could not find target node");
        }
    });

    connect(&AppConfig::instance(), &AppConfig::updated, this, &PipewireAudioService::onAppConfigUpdated);
    //connect(this, &PipewireAudioService::outputDeviceChanged, this, &PipewireAudioService::reloadService);
}

PipewireAudioService::~PipewireAudioService()
{
    mgr.release();
    appMgr.release();
    effects.release();
    delete plugin;
}

void PipewireAudioService::onAppConfigUpdated(const AppConfig::Key &key, const QVariant &value)
{
    switch (key) {
    case AppConfig::AudioOutputDevice: {
        const auto name = value.toString();

        if (name.isEmpty())
        {
            return;
        }

        uint32_t device_id = SPA_ID_INVALID;
        for (const auto& [ts, node2] : mgr.get()->node_map)
        {
            if (node2.name == name.toStdString())
            {
                device_id = node2.device_id;
                break;
            }
        }

        if (device_id != SPA_ID_INVALID)
        {
            for (const auto& device : mgr.get()->list_devices)
            {
                if (device.id == device_id)
                {
                    emit outputDeviceChanged(QString::fromStdString(device.description), name);
                    break;
                }
            }
        }
        else
        {
            util::debug("AudioOutputDevice changed: could not find target node");
        }
        break;
    }
    default:
        break;
    }
}

void PipewireAudioService::update(DspConfig *config)
{
    auto* ptr = plugin->host();

    if(ptr == nullptr)
    {
        util::error("PwJamesDspPlugin is NULL. Cannot update configuration.");
        return;
    }

    plugin->host()->update(config);
}

void PipewireAudioService::reloadService()
{
    effects.get()->disconnect_filters();
    plugin->host()->updateFromCache();
    effects.get()->connect_filters();
}

DspHost *PipewireAudioService::host()
{
    return plugin->host();
}

IAppManager *PipewireAudioService::appManager()
{
    return appMgr.get();
}

std::vector<IOutputDevice> PipewireAudioService::sinkDevices()
{
    std::vector<IOutputDevice> devices;
    for(const auto &[id, node] : mgr.get()->node_map)
    {
        if(node.media_class == "Audio/Sink" && node.name != "jamesdsp_sink"){
            for(const auto &blacklist : mgr.get()->blocklist_node_name){
                if(node.name == blacklist)
                {
                    continue;
                }
            }

            devices.push_back(PwDevice(node));
        }
    }
    return devices;
}

DspStatus PipewireAudioService::status()
{
    return plugin->status();
}

