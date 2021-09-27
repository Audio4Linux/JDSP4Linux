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

    mgr.get()->getDsp()->setMessageHandler([this](DspHost::Message msg, std::any value){
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

