#ifndef PULSEAUDIOSERVICE_H
#define PULSEAUDIOSERVICE_H

#include "PulsePipelineManager.h"

#include <memory>
#include <QObject>

#include "IAudioService.h"

class PulseAudioProcessingThread;

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
    std::vector<IOutputDevice> sinkDevices() override;

    DspStatus status() override;

private:
    std::shared_ptr<PulsePipelineManager> mgr;
    PulseAudioProcessingThread* apt;

};

#endif // PULSEAUDIOSERVICE_H
