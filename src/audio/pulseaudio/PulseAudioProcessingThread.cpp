#include "PulsePipelineManager.h"

#include "PulseAudioProcessingThread.h"
#include "Utils.h"

PulseAudioProcessingThread::PulseAudioProcessingThread(std::shared_ptr<PulsePipelineManager> _mgr)
{
    mgr = _mgr;
    connect(this, &PulseAudioProcessingThread::finished, this, &QObject::deleteLater);
}

PulseAudioProcessingThread::~PulseAudioProcessingThread()
{
    mgr.reset();
}

void PulseAudioProcessingThread::run()
{
    /* Update output sink if na headset change was detected */
    mgr.get()->getPulseManager()->new_default_sink.connect([&](auto name) {
        util::debug("new default sink: " + name);

        if (!name.empty()) {
            mgr.get()->setOutputSinkName(name);
        }
    });

    /* Find all initial sources/sinks */
    mgr.get()->getPulseManager()->find_sink_inputs();
    mgr.get()->getPulseManager()->find_source_outputs();
    mgr.get()->getPulseManager()->find_sinks();

    mgr.get()->link();

    mgr.get()->runLoop();
}

