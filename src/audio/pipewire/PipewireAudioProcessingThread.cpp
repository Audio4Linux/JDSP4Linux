#include "PipewireAudioProcessingThread.h"

PipewireAudioProcessingThread::PipewireAudioProcessingThread(std::shared_ptr<PipewirePipelineManager> _mgr)
{
    mgr = _mgr;
    connect(this, &PipewireAudioProcessingThread::finished, this, &QObject::deleteLater);
}

PipewireAudioProcessingThread::~PipewireAudioProcessingThread()
{
    mgr.reset();
}

void PipewireAudioProcessingThread::run()
{

}

