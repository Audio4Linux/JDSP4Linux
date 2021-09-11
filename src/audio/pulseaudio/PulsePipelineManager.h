#ifndef PULSEPIPELINEMANAGER_H
#define PULSEPIPELINEMANAGER_H

#include <gst/gst.h>
#include <glibmm.h>
#include <glib.h>
#include <sigc++/connection.h>

#include "pipeline/JamesDspElement.h"
#include "pipeline/sink/SinkElement.h"
#include "pipeline/source/SourceElement.h"
#include "PulseManager.h"
#include "RealtimeKit.h"

class PulsePipelineManager
{
public:
    PulsePipelineManager();
    ~PulsePipelineManager();

    PulseManager*       getPulseManager();

    GstElement*         getPipeline() const;
    void                setSource(SourceElement* _source);
    void                setSink(SinkElement *value);
    void                setStreamProperties(const std::string &props);

    void                link();
    void                unlink();
    void                runLoop();
    void                terminateLoop();

    GstElement*         filter;
    GstClockTime        state_check_timeout = 5 * GST_SECOND;
    bool                playing  = false; /* TODO: set to true if appropriate */

    RealtimeKit::
    PriorityType        priority_type = RealtimeKit::pt_none;
    int                 niceness = -10;
    int                 priority = 15;
    RealtimeKit*        rtkit;

    void                setNullPipeline();
    void                updatePipelineState();
    bool                appsWantToPlay();
    SourceElement*      getSource() const;
    SinkElement*        getSink() const;
    JamesDspElement*    getDsp() const;

    void                getLatency();

    void                reallocDsp();

    sigc::signal
    <void, int>         new_latency;
    GMutex*             pipelineLock;

    void                setPulseaudioProps(const std::string &props);
    void                setOutputSinkName(const std::string &name);
    void                setSourceMonitorName(const std::string &name);
    void                pauseAndStart();
    void                setState(GstState state);

    sigc::signal
    <void> snapshot_loaded;
    sigc::signal
    <void, std::map<std::string, std::string>> snapshot_has_missing_plugins;

private:
    PulseManager*       pm;
    GMainLoop*          loop;

    GstBus*             bus;
    guint               bus_watch_id;

    GstElement*         pipeline;
    SourceElement*      source;
    SinkElement*        sink;
    JamesDspElement*    dsp;

    bool                sinkSourceAdded = false;

    uint                current_rate = 0;

    sigc::connection    timeout_connection;

    void                setCaps(const uint &sampling_rate);

    void                onAppAdded(const std::shared_ptr<AppInfo> &app_info);
    void                onAppChanged(const std::shared_ptr<AppInfo> &app_info);
    void                onAppRemoved(uint idx);
    void                onSinkChanged(const std::shared_ptr<mySinkInfo> &sink_info);

    std::vector<std::shared_ptr<AppInfo>> apps_list;
};

#endif // PULSEPIPELINEMANAGER_H
