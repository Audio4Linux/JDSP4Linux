#include <glibmm.h>

#include "PipewireAudioService.h"
#include "PwJamesDspPlugin.h"

#include "PwPipelineManager.h"
#include "DspHost.h"
#include "PwDevice.h"
#include "Utils.h"
#include "config/AppConfig.h"

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
        if (device.output_route_available == SPA_PARAM_AVAILABILITY_no) {
            return;
        }

        util::debug(log_tag + "device " + device.name + " has changed its output route to: " + device.output_route_name);
    });
}

PipewireAudioService::~PipewireAudioService()
{
    delete plugin;
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

