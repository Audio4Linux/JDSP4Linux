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

    mgr = std::make_unique<PwPipelineManager>();
    appMgr = std::make_unique<PwAppManager>(mgr.get());
    plugin = new PwJamesDspPlugin(mgr.get());
    effects = std::make_unique<FilterContainer>(mgr.get(), plugin, &AppConfig::instance());

    plugin->setMessageHandler(std::bind(&IAudioService::handleMessage, this, std::placeholders::_1, std::placeholders::_2));

    mgr.get()->new_default_sink.connect([&](NodeInfo node) {
        util::debug(log_tag + "new default output device: " + node.name);

        if (AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault)) {
            /*
            Depending on the hardware headphones can cause a node recreation where the id and the name are kept.
            So we clear the key to force the callbacks to be called
          */

            AppConfig::instance().set(AppConfig::AudioOutputDevice, "");
            AppConfig::instance().set(AppConfig::AudioOutputDevice, QString::fromStdString(node.name));
        }
    });

    mgr.get()->device_output_route_changed.connect([&](DeviceInfo device) {
        if (device.output_route_available == SPA_PARAM_AVAILABILITY_no)
        {
            return;
        }

        NodeInfo target_node;
        for (const auto& [ts, node] : mgr.get()->node_map)
        {
            target_node = node;

            if (node.device_id == device.id && node.media_class == "Audio/Sink")
            {
                target_node = node;
                break;
            }
        }

        if (target_node.id != SPA_ID_INVALID)
        {
            emit outputDeviceChanged(QString::fromStdString(target_node.name), QString::fromStdString(device.output_route_name));
        }
        else
        {
            util::debug(log_tag + "device_output_route_changed: could not find target node");
        }
    });

    connect(&AppConfig::instance(), &AppConfig::updated, this, &PipewireAudioService::onAppConfigUpdated);
}

PipewireAudioService::~PipewireAudioService()
{
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

        uint device_id = SPA_ID_INVALID;
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
                    emit outputDeviceChanged(name, QString::fromStdString(device.output_route_name));
                    break;
                }
            }
        }
        else
        {
            util::debug(log_tag + "AudioOutputDevice changed: could not find target node");
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
        util::error("PipewireAudioService::update: PwJamesDspPlugin is NULL. Cannot update configuration.");
        return;
    }

    plugin->host()->update(config);
}

void PipewireAudioService::reloadLiveprog()
{
    plugin->host()->reloadLiveprog();
}

void PipewireAudioService::reloadService()
{
    effects.get()->disconnect_filters();
    plugin->host()->updateFromCache();
    effects.get()->connect_filters();
}

IAppManager *PipewireAudioService::appManager()
{
    return appMgr.get();
}

std::vector<IOutputDevice> PipewireAudioService::sinkDevices()
{
    std::vector<IOutputDevice> devices;
    for(auto [id, node] : mgr.get()->node_map)
    {
        if(node.media_class == "Audio/Sink" && node.name != "jamesdsp_sink"){
            for(auto blacklist : mgr.get()->blocklist_node_name){
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

#include <iostream>

#include <config/AppConfig.h>
void PipewireAudioService::enumerateLiveprogVariables()
{

    // TODO
    auto vars = plugin->host()->enumEelVariables();

    for(const auto& var : vars)
    {
        if(std::holds_alternative<std::string>(var.value))
            std::cout << std::boolalpha << var.name << "\t\t" << std::get<std::string>(var.value) << "\t" << var.isString << std::endl;
        else
            std::cout << std::boolalpha << var.name << "\t\t" << std::get<float>(var.value) << "\t" << var.isString << std::endl;
    }

    std::cout << "-------------------" << std::endl;

    emit eelVariablesEnumerated(vars);
}

