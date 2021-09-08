#ifndef PULSEAUDIOPROCESSINGTHREAD_H
#define PULSEAUDIOPROCESSINGTHREAD_H

#include "PulsePipelineManager.h"
#include "PulseManager.h"

#include <QThread>
#include <QMutex>

class PulseAudioProcessingThread : public QThread
{
    Q_OBJECT
public:
    PulseAudioProcessingThread(std::shared_ptr<PulsePipelineManager> _mgr);
    ~PulseAudioProcessingThread();

protected:
   virtual void run();

private:
    std::shared_ptr<PulsePipelineManager> mgr;
    bool abort = false;
};

#endif // PULSEAUDIOPROCESSINGTHREAD_H
