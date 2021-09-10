#ifndef PWAUDIOPROCESSINGTHREAD_H
#define PWAUDIOPROCESSINGTHREAD_H

#include "PwPipelineManager.h"

#include <QThread>
#include <QMutex>

class PwAudioProcessingThread : public QThread
{
    Q_OBJECT
public:
    PwAudioProcessingThread(std::shared_ptr<PwPipelineManager> _mgr);
    ~PwAudioProcessingThread();

protected:
   virtual void run();

private:
    std::shared_ptr<PwPipelineManager> mgr;
    bool abort = false;
};

#endif // PWAUDIOPROCESSINGTHREAD_H
