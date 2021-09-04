#include "PulseAudioService.h"

#include "Utils.h"
#include "PipelineManager.h"
#include "AudioProcessingThread.h"

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
     mgr = std::make_shared<PipelineManager>();

     /* Launch audio processing thread */
     apt = new AudioProcessingThread(mgr);
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

    mgr.get()->getDsp()->update(config);
}

void PulseAudioService::reloadLiveprog()
{
    mgr.get()->getDsp()->reloadLiveprog();
}

void PulseAudioService::reloadService()
{
    mgr.get()->unlink();
    mgr.get()->link();
}

