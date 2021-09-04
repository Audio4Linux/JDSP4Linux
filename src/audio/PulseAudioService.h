#ifndef PULSEAUDIOSERVICE_H
#define PULSEAUDIOSERVICE_H

#include "PipelineManager.h"

#include <memory>
#include <QObject>

#include "IAudioService.h"

class AudioProcessingThread;

class PulseAudioService : public IAudioService
{
    Q_OBJECT
    Q_INTERFACES(IAudioService)

public:
    PulseAudioService();
    ~PulseAudioService();

public slots:
    void update(DspConfig* config) override;
    void reloadLiveprog() override;
    void reloadService() override;

private:
    std::shared_ptr<PipelineManager> mgr;
    AudioProcessingThread* apt;

};

#endif // PULSEAUDIOSERVICE_H
