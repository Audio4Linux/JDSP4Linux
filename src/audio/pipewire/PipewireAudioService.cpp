#include <glibmm.h>

#include "PipewireAudioService.h"

#include "PipewireAudioProcessingThread.h"
#include "PipewirePipelineManager.h"
#include "DspHost.h"
#include "Utils.h"

#include <QDebug>

PipewireAudioService::PipewireAudioService()
{
    Glib::init();

    /* Create a shared thread-safe pointer */
    mgr = std::make_shared<PipewirePipelineManager>();

    /*mgr.get()->getDsp()->setMessageHandler([this](DspHost::Message msg, std::any value){
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
    });*/

    /* Launch audio processing thread */
    apt = new PipewireAudioProcessingThread(mgr);
    apt->start();
}

PipewireAudioService::~PipewireAudioService()
{
    //mgr.get()->terminateLoop();
    if(!apt->wait(300)){
        apt->terminate();
    }
    delete apt;

    /* Release ownership from main */
    mgr.reset();

    /* Make sure the PipelineManager is not referenced anymore and was destroyed */
    assert(mgr.use_count() == 0);
}

void PipewireAudioService::update(DspConfig *config)
{
    //auto* ptr = mgr.get()->getDsp();

    /*if(ptr == nullptr)
    {
        util::error("PipewireAudioService::update: JamesDspElement is NULL. Cannot update configuration.");
        return;
    }*/

    //mgr.get()->getDsp()->host()->update(config);
}

void PipewireAudioService::reloadLiveprog()
{
    //mgr.get()->getDsp()->host()->reloadLiveprog();
}

void PipewireAudioService::reloadService()
{
    //mgr.get()->reallocDsp();
}

