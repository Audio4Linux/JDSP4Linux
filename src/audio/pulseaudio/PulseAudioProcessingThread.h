#ifndef PULSEAUDIOPROCESSINGTHREAD_H
#define PULSEAUDIOPROCESSINGTHREAD_H

#include <QThread>
#include <QMutex>

class PulsePipelineManager;
typedef std::shared_ptr<PulsePipelineManager> PulsePipelineManagerPtr;

class PulseAudioProcessingThread : public QThread
{
    Q_OBJECT
public:
    PulseAudioProcessingThread(PulsePipelineManagerPtr _mgr);
    ~PulseAudioProcessingThread();

protected:
   virtual void run();

private:
    PulsePipelineManagerPtr mgr;
    bool abort = false;
};

#endif // PULSEAUDIOPROCESSINGTHREAD_H
