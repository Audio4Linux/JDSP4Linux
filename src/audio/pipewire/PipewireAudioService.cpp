#include <glibmm.h>

#include "PipewireAudioService.h"

#include "PwAudioProcessingThread.h"
#include "PwPipelineManager.h"
#include "DspHost.h"
#include "Utils.h"
#include "config/AppConfig.h"

#include <QDebug>

PipewireAudioService::PipewireAudioService()
{
    Glib::init();

    connect(&AppConfig::instance(), &AppConfig::updated, this, &PipewireAudioService::notifyConfigChange);

    initialize();
}

PipewireAudioService::~PipewireAudioService()
{

}

void PipewireAudioService::initialize()
{
    mgr = std::make_unique<PwPipelineManager>();
    plugin = std::make_unique<PwJamesDspPlugin>(mgr.get());
    effects = std::make_unique<FilterContainer>(mgr.get(), plugin.get(), &AppConfig::instance());

    plugin.get()->setMessageHandler([this](DspHost::Message msg, std::any value){
        switch(msg)
        {
            case DspHost::EelCompilerResult: {
                auto args = std::any_cast<QList<QString>>(value);
                int ret = args[0].toInt();

                emit eelCompilationFinished(ret, checkErrorCode(ret), args[1], args[2], args[3].toFloat());
                break;
            }
            case DspHost::EelCompilerStart:
                emit eelCompilationStarted(std::any_cast<QString>(value));
                break;
            case DspHost::EelWriteOutputBuffer:
                emit eelOutputReceived(std::any_cast<QString>(value));
                break;
            default:
                break;
        }
    });


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

    //settings->signal_changed("bypass").connect([=, this](const auto& key) { update_bypass_state(key); });

    //update_bypass_state("bypass");

}

void PipewireAudioService::notifyConfigChange(const AppConfig::Key& key, const QVariant& value)
{
    switch(key)
    {
        case AppConfig::AudioOutputDevice:


        break;
    }
}

void PipewireAudioService::update(DspConfig *config)
{
    auto* ptr = plugin.get()->host();

    if(ptr == nullptr)
    {
        util::error("PipewireAudioService::update: PwJamesDspPlugin is NULL. Cannot update configuration.");
        return;
    }

    plugin.get()->host()->update(config);
}

void PipewireAudioService::reloadLiveprog()
{
    plugin.get()->host()->reloadLiveprog();
}

void PipewireAudioService::reloadService()
{   
    effects.get()->disconnect_filters();
    effects.get()->connect_filters();
}

