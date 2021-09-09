#ifndef PIPEWIREAUDIOPROCESSINGTHREAD_H
#define PIPEWIREAUDIOPROCESSINGTHREAD_H

#include "PipewirePipelineManager.h"

#include <QThread>
#include <QMutex>



class PipewireAudioProcessingThread : public QThread
{
    Q_OBJECT
public:
    PipewireAudioProcessingThread(std::shared_ptr<PipewirePipelineManager> _mgr);
    ~PipewireAudioProcessingThread();

protected:
   virtual void run();

private:
    std::shared_ptr<PipewirePipelineManager> mgr;
    bool abort = false;
};

#endif // PIPEWIREAUDIOPROCESSINGTHREAD_H
