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
    /* Register sink change handler */
    std::string last_sink_dev_name;

    mgr.get()->getPulseManager()->sink_changed.connect([&](const std::shared_ptr<mySinkInfo>& info) {
        if (info->name == mgr.get()->getPulseManager()->server_info.default_sink_name) {
            Glib::signal_timeout().connect_seconds_once(
                        [=,&last_sink_dev_name]() {
                // checking if after 3 seconds this sink still is the default sink
                if (info->name == mgr.get()->getPulseManager()->server_info.default_sink_name) {
                    auto current_info = mgr.get()->getPulseManager()->get_sink_info(info->name);

                    if (current_info != nullptr) {
                        auto port = current_info->active_port;
                        std::string dev_name;

                        if (port != "null") {
                            dev_name = current_info->name + ":" + port;
                        } else {
                            dev_name = current_info->name;
                        }

                        if (dev_name != last_sink_dev_name) {
                            last_sink_dev_name = dev_name;

                            //presets_manager->autoload(PresetType::output, dev_name);
                        }
                    }
                }
            },
            3);
        }
    });

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

