#include "PwAudioProcessingThread.h"

PwAudioProcessingThread::PwAudioProcessingThread(std::shared_ptr<PwPipelineManager> _mgr)
{
    mgr = _mgr;
    connect(this, &PwAudioProcessingThread::finished, this, &QObject::deleteLater);
}

PwAudioProcessingThread::~PwAudioProcessingThread()
{
    mgr.reset();
}

void PwAudioProcessingThread::run()
{

}

