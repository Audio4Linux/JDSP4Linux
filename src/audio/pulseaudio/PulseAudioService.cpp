#include "PulsePipelineManager.h"

#include "PulseAudioService.h"

#include "PulseAppManager.h"
#include "Utils.h"
#include "DspHost.h"
#include "PulseAudioProcessingThread.h"
#include "pipeline/JamesDspElement.h"

#include <QDebug>

#include <gstjamesdsp.h>

PulseAudioService::PulseAudioService()
{
    Glib::init();
    gst_init (nullptr, nullptr);

    bool reg = gst_plugin_register_static(GST_VERSION_MAJOR,
                                          GST_VERSION_MINOR,
                                          "jamesdsp",
                                          "JamesDSP element",
                                          jamesdsp_init,
                                          VERSION,
                                          "GPL",
                                          "GStreamer",
                                          "JamesDSP for Linux",
                                          "https://github.com/james34602/JamesDSPManager");

    if(!reg)
    {
        util::error("gst_plugin_register_static: Unable to register internal JamesDSP plugin from memory. Aborting...");
        abort();
    }

    /* Create a shared thread-safe pointer */
    mgr = std::make_shared<PulsePipelineManager>();
    appMgr = new PulseAppManager(mgr.get());

    mgr.get()->getDsp()->setMessageHandler(std::bind(&IAudioService::handleMessage, this, std::placeholders::_1, std::placeholders::_2));

    /* Register sink change handler */
    mgr.get()->getPulseManager()->sink_changed.connect([&](const std::shared_ptr<mySinkInfo>& info) {
        if (info->name == mgr.get()->getPulseManager()->server_info.default_sink_name) {
            Glib::signal_timeout().connect_seconds_once(
                        [=]() {
                // checking if after 3 seconds this sink still is the default sink
                if (info->name == mgr.get()->getPulseManager()->server_info.default_sink_name) {
                    auto current_info = mgr.get()->getPulseManager()->get_sink_info(info->name);

                    if (current_info != nullptr) {
                        auto port = current_info->active_port;
                        std::string dev_name;

                        if (port != "null") {
                            dev_name = current_info->name + ":" + port;
                        } else {
                            dev_name = current_info->name;
                        }

                        if (dev_name != last_sink_dev_name) {
                            last_sink_dev_name = dev_name;
                            emit outputDeviceChanged(QString::fromStdString(dev_name), QString::fromStdString(dev_name));
                        }
                    }
                }
            },
            3);
        }
    });

    /* Launch audio processing thread */
    apt = new PulseAudioProcessingThread(mgr);
    apt->start();
}

PulseAudioService::~PulseAudioService()
{
    mgr.get()->terminateLoop();
    if(!apt->wait(300)){
        apt->terminate();
    }
    delete apt;

    /* Release ownership from main */
    mgr.reset();

    /* Make sure the PipelineManager is not referenced anymore and was destroyed */
    assert(mgr.use_count() == 0);
}

void PulseAudioService::update(DspConfig *config)
{
    auto* ptr = mgr.get()->getDsp();

    if(ptr == nullptr)
    {
        util::error("PulseAudioService::update: JamesDspElement is NULL. Cannot update configuration.");
        return;
    }

    mgr.get()->getDsp()->host()->update(config);
}

void PulseAudioService::reloadLiveprog()
{
    mgr.get()->getDsp()->host()->reloadLiveprog();
}

void PulseAudioService::reloadService()
{
    mgr.get()->relink();
    mgr.get()->getDsp()->host()->updateFromCache();
}

IAppManager *PulseAudioService::appManager()
{
    return appMgr;
}

std::vector<IOutputDevice> PulseAudioService::sinkDevices()
{
    return std::vector<IOutputDevice>();
}

DspStatus PulseAudioService::status()
{
    return mgr.get()->getDsp()->status();
}

void PulseAudioService::enumerateLiveprogVariables()
{
    auto vars = mgr.get()->getDsp()->host()->enumEelVariables();
    emit eelVariablesEnumerated(vars);
}

