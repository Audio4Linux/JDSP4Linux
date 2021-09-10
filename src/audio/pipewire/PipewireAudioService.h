#ifndef PIPEWIREAUDIOSERVICE_H
#define PIPEWIREAUDIOSERVICE_H

#include "PwPipelineManager.h"
#include "FilterContainer.h"
#include "IAudioService.h"
#include "config/AppConfig.h"

#include "PwJamesDspPlugin.h"

#include <memory>
#include <QObject>

class PwAudioProcessingThread;

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

private slots:
    void notifyConfigChange(const AppConfig::Key& key, const QVariant& value);

private:
    const std::string log_tag = "PipewireAudioService: ";

    std::unique_ptr<PwPipelineManager> mgr;
    std::unique_ptr<FilterContainer> effects;
    std::unique_ptr<PwJamesDspPlugin> plugin;

    void initialize();
};

#endif // PIPEWIREAUDIOSERVICE_H
