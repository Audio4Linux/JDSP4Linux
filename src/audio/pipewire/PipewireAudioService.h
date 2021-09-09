#ifndef PIPEWIREAUDIOSERVICE_H
#define PIPEWIREAUDIOSERVICE_H

#include "PipewirePipelineManager.h"
#include "IAudioService.h"

#include <memory>
#include <QObject>

class PipewireAudioProcessingThread;

class PipewireAudioService : public IAudioService
{
    Q_OBJECT
    Q_INTERFACES(IAudioService)

public:
    PipewireAudioService();
    ~PipewireAudioService();

public slots:
    void update(DspConfig* config) override;
    void reloadLiveprog() override;
    void reloadService() override;

private:
    std::shared_ptr<PipewirePipelineManager> mgr;
    PipewireAudioProcessingThread* apt;

};

#endif // PIPEWIREAUDIOSERVICE_H
