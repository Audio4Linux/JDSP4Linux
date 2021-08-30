#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "PipelineManager.h"

#include <memory>
#include <QObject>

class AudioProcessingThread;

class AudioManager : QObject
{
    Q_OBJECT
public:
    AudioManager();
    ~AudioManager();

signals:
    void eelCompilationFinished(int ret);
    void linkingFailed();

private:
    std::shared_ptr<PipelineManager> mgr;
    AudioProcessingThread* apt;
};

#endif // AUDIOMANAGER_H
