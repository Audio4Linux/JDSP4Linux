#ifndef PULSEAUDIOSERVICE_H
#define PULSEAUDIOSERVICE_H

#include <memory>

#include "IAudioService.h"

class PulseAppManager;
class PulseAudioProcessingThread;

class PulsePipelineManager;
typedef std::shared_ptr<PulsePipelineManager> PulsePipelineManagerPtr;

class PulseAudioService : public IAudioService
{
    Q_OBJECT
    Q_INTERFACES(IAudioService)

public:
    PulseAudioService();
    ~PulseAudioService();

public slots:
    void update(DspConfig* config) override;
    void reloadService() override;

    DspHost* host() override;
    IAppManager* appManager() override;
    std::vector<IOutputDevice> sinkDevices() override;
    DspStatus status() override;

private:
    PulsePipelineManagerPtr mgr;
    PulseAppManager* appMgr;
    PulseAudioProcessingThread* apt;

    std::string last_sink_dev_name;
    std::map<uint, mySinkInfo> sinks;

};

#endif // PULSEAUDIOSERVICE_H
