#ifndef AUDIOPROCESSINGTHREAD_H
#define AUDIOPROCESSINGTHREAD_H

#include "PipelineManager.h"
#include "PulseManager.h"

#include <QThread>
#include <QMutex>

class AudioProcessingThread : public QThread
{
    Q_OBJECT
public:
    AudioProcessingThread(std::shared_ptr<PipelineManager> _mgr);
    ~AudioProcessingThread();

protected:
   virtual void run();

private:
    std::shared_ptr<PipelineManager> mgr;
    bool abort = false;
};

#endif // AUDIOPROCESSINGTHREAD_H
