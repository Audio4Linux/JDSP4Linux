#include "PulsePipelineManager.h"

#include "pipeline/sink/PulseSinkElement.h"
#include "pipeline/source/PulseSrcElement.h"
#include "pipeline/JamesDspElement.h"
#include "pipeline/sink/SinkElement.h"
#include "pipeline/source/SourceElement.h"

#include "Utils.h"

#include <pipeline/GstElementProperties.h>

#include <config/AppConfig.h>

static void on_src_pad_added   (GstElement *element,
                                GstPad     *pad,
                                gpointer    data);

static void on_sink_pad_added  (GstElement *element,
                                GstPad     *pad,
                                gpointer    data);

void on_message_error([[maybe_unused]] const GstBus* gst_bus, GstMessage* message, PulsePipelineManager* p) {
    GError* err;
    gchar* debug;

    gst_message_parse_error(message, &err, &debug);

    util::critical(err->message);
    util::debug(debug);

    p->setNullPipeline();

    g_error_free(err);
    g_free(debug);
}

void on_stream_status([[maybe_unused]] GstBus* bus, GstMessage* message, PulsePipelineManager* p) {
    GstStreamStatusType type;
    GstElement* owner;
    gchar* path;
    std::string path_str;
    std::string source_name;
    std::size_t idx;

    gst_message_parse_stream_status(message, &type, &owner);

    switch (type) {
    case GST_STREAM_STATUS_TYPE_ENTER:
        path = gst_object_get_path_string(GST_OBJECT(owner));

        path_str = path;

        idx = path_str.find_last_of('/');

        source_name = path_str.substr(idx + 1);

        g_free(path);

        if (p->priority_type == RealtimeKit::pt_niceness) {;
            p->rtkit->set_nice(source_name, p->niceness);
        } else if (p->priority_type == RealtimeKit::pt_realtime) {
            p->rtkit->set_priority(source_name, p->priority);
        }

        break;
    default:
        break;
    }
}

void on_buffer_changed([[maybe_unused]] GObject* gobject, [[maybe_unused]] GParamSpec* pspec, PulsePipelineManager* p) {
    GstState state;
    GstState pending;

    gst_element_get_state(p->getPipeline(), &state, &pending, p->state_check_timeout);

    if (state == GST_STATE_PLAYING || state == GST_STATE_PAUSED) {
        /* when we are playing it is necessary to reset the pipeline for the new
         * value to take effect */

        gst_element_set_state(p->getPipeline(), GST_STATE_READY);

        p->updatePipelineState();
    }
}

void on_message_state_changed([[maybe_unused]] const GstBus* gst_bus, GstMessage* message, PulsePipelineManager* p) {
    if (std::strcmp(GST_OBJECT_NAME(message->src), "pipeline") == 0) {
        GstState old_state;
        GstState new_state;
        GstState pending;

        gst_message_parse_state_changed(message, &old_state, &new_state, &pending);

        util::debug(std::string(gst_element_state_get_name(old_state)) + " -> " + gst_element_state_get_name(new_state) +
                    " -> " + gst_element_state_get_name(pending));

        if (new_state == GST_STATE_PLAYING) {
            p->playing = true;

            p->getLatency();
        } else {
            p->playing = false;
        }
    }
}

void on_message_latency([[maybe_unused]] const GstBus* gst_bus, GstMessage* message, PulsePipelineManager* p) {
    if (std::strcmp(GST_OBJECT_NAME(message->src), "source") == 0) {
        int latency;
        int buffer;

        g_object_get(p->getSource()->getGstElement(), "latency-time", &latency, nullptr);
        g_object_get(p->getSource()->getGstElement(), "buffer-time", &buffer, nullptr);

        util::debug("pulsesrc latency [us]: " + std::to_string(latency));
        util::debug("pulsesrc buffer [us]: " + std::to_string(buffer));
    } else if (std::strcmp(GST_OBJECT_NAME(message->src), "sink") == 0) {
        int latency;
        int buffer;

        g_object_get(p->getSink()->getGstElement(), "latency-time", &latency, nullptr);
        g_object_get(p->getSink()->getGstElement(), "buffer-time", &buffer, nullptr);

        util::debug("pulsesink latency [us]: " + std::to_string(latency));
        util::debug("pulsesink buffer [us]: " + std::to_string(buffer));
    }

    p->getLatency();
}

void on_latency_changed([[maybe_unused]] GObject* gobject, [[maybe_unused]] GParamSpec* pspec, PulsePipelineManager* p) {
    GstState state;
    GstState pending;

    gst_element_get_state(p->getPipeline(), &state, &pending, p->state_check_timeout);

    if (state == GST_STATE_PLAYING || state == GST_STATE_PAUSED) {
        /* when we are playing it is necessary to reset the pipeline for the new
         * value to take effect */

        gst_element_set_state(p->getPipeline(), GST_STATE_READY);

        p->updatePipelineState();
    }
}

auto on_sink_event([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer user_data) -> GstPadProbeReturn {
    GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);

    if (event->type == GST_EVENT_LATENCY) {
        auto p = static_cast<PulsePipelineManager*>(user_data);
        p->getLatency();
    }

    return GST_PAD_PROBE_PASS;
}

PulsePipelineManager::PulsePipelineManager()
{
    /* Initialisation */
    loop     = g_main_loop_new (NULL, FALSE);
    rtkit    = new RealtimeKit();
    pm       = new PulseManager();
    dsp      = new JamesDspElement();

    /* Create a new pipeline */
    pipeline = gst_pipeline_new ("pipeline");

    bus = gst_element_get_bus(pipeline);
    gst_bus_enable_sync_message_emission(bus);
    gst_bus_add_signal_watch(bus);

    g_signal_connect(bus, "message::error", G_CALLBACK(on_message_error), this);
    g_signal_connect(bus, "sync-message::stream-status", GCallback(on_stream_status), this);
    g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_message_state_changed), this);
    g_signal_connect(bus, "message::latency", G_CALLBACK(on_message_latency), this);

    pm->sink_input_added.connect(sigc::mem_fun(*this, &PulsePipelineManager::onAppAdded));
    pm->sink_input_changed.connect(sigc::mem_fun(*this, &PulsePipelineManager::onAppChanged));
    pm->sink_input_removed.connect(sigc::mem_fun(*this, &PulsePipelineManager::onAppRemoved));
    pm->sink_changed.connect(sigc::mem_fun(*this, &PulsePipelineManager::onSinkChanged));

    setSource(new PulseSrcElement());
    setSink(new PulseSinkElement());

    GstPad* sinkpad = sink->getSinkPad();
    gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, on_sink_event, this, nullptr);
    g_object_unref(sinkpad);

    std::string pulse_props = "application.id=com.github.gstmgr";

    setPulseaudioProps(pulse_props);

    setSourceMonitorName(pm->apps_sink_info->monitor_source_name);
    setCaps(pm->apps_sink_info->rate);

    auto* PULSE_SINK = std::getenv("PULSE_SINK");

    if (PULSE_SINK != nullptr) {
        if (pm->get_sink_info(PULSE_SINK)) {
            setOutputSinkName(PULSE_SINK);
        } else {
            setOutputSinkName(pm->server_info.default_sink_name);
        }
    } else {
        bool use_default_sink = true;//g_settings_get_boolean(settings, "use-default-sink") != 0;

        if (use_default_sink) {
            setOutputSinkName(pm->server_info.default_sink_name);
        } else {
            gchar* custom_sink = NULL;//g_settings_get_string(settings, "custom-sink");

            if (pm->get_sink_info(custom_sink)) {
                setOutputSinkName(custom_sink);
            } else {
                setOutputSinkName(pm->server_info.default_sink_name);
            }

            g_free(custom_sink);
        }
    }
}

PulsePipelineManager::~PulsePipelineManager(){
    timeout_connection.disconnect();

    util::debug ("Stopping playback");
    setNullPipeline();

    util::debug ("Deleting pipeline");
    gst_object_unref (GST_OBJECT (pipeline));

    gst_object_unref (bus);
    g_main_loop_unref(loop);

    util::debug ("Deleting jamesdsp object");
    delete dsp;

    util::debug ("Deleting pulse manager");
    delete pm;
}

PulseManager* PulsePipelineManager::getPulseManager()
{
    return pm;
}

void PulsePipelineManager::setSource(SourceElement* _source){
    source = _source;
    if(source->getType() == SourceElement::Pulse){
        g_signal_connect(source->getGstElement(), "notify::buffer-time", G_CALLBACK(on_buffer_changed), this);
        g_signal_connect(source->getGstElement(), "notify::latency-time", G_CALLBACK(on_latency_changed), this);
    }
}

void PulsePipelineManager::setSink(SinkElement *value)
{
    sink = value;
    if(sink->getType() == SinkElement::Pulse){
        g_signal_connect(sink->getGstElement(), "notify::buffer-time", G_CALLBACK(on_buffer_changed), this);
        g_signal_connect(sink->getGstElement(), "notify::latency-time", G_CALLBACK(on_latency_changed), this);
    }
}

void PulsePipelineManager::setStreamProperties(const std::string& props)
{
    auto str = "props," + props;

    auto s = gst_structure_from_string(str.c_str(), nullptr);

    source->setValues("stream-properties", s, NULL);
    sink->setValues("stream-properties", s, NULL);

    gst_structure_free(s);
}

void PulsePipelineManager::setNullPipeline() {
    gst_element_set_state(pipeline, GST_STATE_NULL);

    GstState state;
    GstState pending;

    gst_element_get_state(pipeline, &state, &pending, state_check_timeout);

    /* on_message_state is not called when going to null. I don't know why.
     * so we have to update the variable manually after setting to null. */

    if (state == GST_STATE_NULL) {
        playing = false;
    }

    util::debug(std::string(gst_element_state_get_name(state)) + " -> " + gst_element_state_get_name(pending));

}

void PulsePipelineManager::updatePipelineState() {
    bool wants_to_play = appsWantToPlay();

    GstState state;
    GstState pending;

    gst_element_get_state(pipeline, &state, &pending, state_check_timeout);

    if (state != GST_STATE_PLAYING && wants_to_play) {
        timeout_connection.disconnect();

        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    } else if (state == GST_STATE_PLAYING && !wants_to_play) {
        timeout_connection.disconnect();

        timeout_connection = Glib::signal_timeout().connect_seconds(
                    [=]() {
            GstState s;
            GstState p;

            gst_element_get_state(pipeline, &s, &p, state_check_timeout);

            if (s == GST_STATE_PLAYING && !appsWantToPlay()) {
                util::debug("No app wants to play audio. We will pause our pipeline.");

                gst_element_set_state(pipeline, GST_STATE_PAUSED);
            }

            return false;
        },
        5);
    }
}

void PulsePipelineManager::pauseAndStart(){
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void PulsePipelineManager::setState(GstState state){
    gst_element_set_state(pipeline, state);
}
/**
 * @note Only applies if the source element supports device selection
 */
void PulsePipelineManager::setSourceMonitorName(const std::string& name) {
    gchar* current_device = NULL;

    g_object_get(source->getGstElement(), "current-device", &current_device, nullptr);

    if(current_device == NULL) {
        return;
    }

    if (name != current_device) {
        if (playing) {
            setNullPipeline();

            g_object_set(source->getGstElement(), "device", name.c_str(), nullptr);

            Glib::signal_timeout().connect_seconds_once(
                        [=]() {
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            },
            1);
        } else {
            g_object_set(source->getGstElement(), "device", name.c_str(), nullptr);
        }

        util::debug("using input device: " + name);
    }

    g_free(current_device);
}

/**
 * @note Only applies if the sink element supports device selection
 */
void PulsePipelineManager::setOutputSinkName(const std::string& name) {
    g_object_set(sink->getGstElement(), "device", name.c_str(), nullptr);

    util::debug("using output device: " + name);
}

void PulsePipelineManager::setPulseaudioProps(const std::string& props) {
    auto str = "props," + props;

    auto s = gst_structure_from_string(str.c_str(), nullptr);

    g_object_set(source->getGstElement(), "stream-properties", s, nullptr);
    g_object_set(sink->getGstElement(), "stream-properties", s, nullptr);

    gst_structure_free(s);
}

void PulsePipelineManager::getLatency() {
    GstQuery* q = gst_query_new_latency();

    if (gst_element_query(pipeline, q) != 0) {
        gboolean live;
        GstClockTime min;
        GstClockTime max;

        gst_query_parse_latency(q, &live, &min, &max);

        int latency = GST_TIME_AS_MSECONDS(min);

        util::debug("total latency: " + std::to_string(latency) + " ms");

        Glib::signal_idle().connect_once([=] { new_latency(latency); });
    }

    gst_query_unref(q);
}

void PulsePipelineManager::relink()
{
    unlink();

    link();
}

bool PulsePipelineManager::appsWantToPlay() {
    bool wants_to_play = false;

    for (const auto& a : apps_list) {
        if (a->wants_to_play) {
            wants_to_play = true;

            break;
        }
    }

    return wants_to_play;
}

void PulsePipelineManager::setCaps(const uint32_t& sampling_rate) {
    current_rate = sampling_rate;

    /*auto caps_str = "audio/x-raw,format=F32LE,channels=2,rate=" + std::to_string(sampling_rate);

  auto caps = gst_caps_from_string(caps_str.c_str());

  g_object_set(capsfilter, "caps", caps, nullptr);

  gst_caps_unref(caps);*/
}

void PulsePipelineManager::onAppAdded(const std::shared_ptr<AppInfo>& app_info) {
    bool alreadyAdded = false;
    for (const auto& a : apps_list) {
        if (a->index == app_info->index) {
            alreadyAdded = true;
        }
    }

    if(!alreadyAdded)
        apps_list.emplace_back(app_info);

    updatePipelineState();

    bool success = false;
    bool is_blocklisted = AppConfig::instance().isAppBlocked(QString::fromStdString(app_info.get()->name));

    if (app_info->connected) {
        if (is_blocklisted) {
            success = pm->remove_sink_input_from_gstmgr(app_info->name, app_info->index);

            if (success) {
                app_info->connected = false;
            }
        }
    } else {
        auto enable_all = true;//g_settings_get_boolean(settings, "enable-all-sinkinputs");

        if (!is_blocklisted && enable_all != 0) {
            success = pm->move_sink_input_to_gstmgr(app_info->name, app_info->index);

            if (success) {
                app_info->connected = true;
            }
        }
    }
}

void PulsePipelineManager::onAppChanged(const std::shared_ptr<AppInfo>& app_info) {
    std::replace_copy_if(
                apps_list.begin(), apps_list.end(), apps_list.begin(), [=](auto& a) { return a->index == app_info->index; },
    app_info);

    updatePipelineState();
}

void PulsePipelineManager::onAppRemoved(uint32_t idx) {
    apps_list.erase(std::remove_if(apps_list.begin(), apps_list.end(), [=](auto& a) { return a->index == idx; }),
                    apps_list.end());

    updatePipelineState();
}

void PulsePipelineManager::onSinkChanged(const std::shared_ptr<mySinkInfo>& sink_info) {
    if (sink_info->name == SINK_NAME) {
        if (sink_info->rate != current_rate) {
            gst_element_set_state(pipeline, GST_STATE_READY);

            setCaps(sink_info->rate);

            updatePipelineState();
        }
    }
}

SourceElement *PulsePipelineManager::getSource() const
{
    return source;
}

SinkElement *PulsePipelineManager::getSink() const
{
    return sink;
}

JamesDspElement *PulsePipelineManager::getDsp() const
{
    return dsp;
}

/**
 * @brief PipelineManager::unlink
 * @note Call this after performing changes to the filter container and before \c link(). Free all removed elements after this call
 */
void PulsePipelineManager::unlink(){
    assert(source != NULL);
    assert(dsp != NULL);
    assert(sink != NULL);

    std::vector<GstElement*> elements = source->getPartialPipeline();
    elements = util::concat_vectors<GstElement*>(elements, dsp->getPartialPipeline());
    elements = util::concat_vectors<GstElement*>(elements, sink->getPartialPipeline());

    setNullPipeline();
    /* unlink everything */
    {
        GstElement* prev = NULL;

        for(auto element : elements){
            if(prev != NULL){
                util::debug("Unlinking previous element '" + std::string(GST_OBJECT_NAME(prev)) + "' from current element '" + GST_OBJECT_NAME(element) + "'");
                assert(GST_IS_OBJECT(prev));
                assert(GST_IS_OBJECT(element));
                gst_pad_push_event(gst_element_get_static_pad(prev, "src"),
                                   gst_event_new_eos());
                gst_element_unlink(prev, element);
            }

            prev = element;
        }
    }

    /* remove everything from the bin */
    {
        for(auto element : elements){
            util::debug("Removing element '" + std::string(GST_OBJECT_NAME(element)) + "' from bin");
            gst_bin_remove(GST_BIN(pipeline), element);
        }
    }

}


void PulsePipelineManager::link(){
    assert(source != NULL);
    assert(dsp != NULL);
    assert(sink != NULL);

    /* Add the elements to the pipeline and link them together */
    {
        std::vector<GstElement*> elements = source->getPartialPipeline();
        elements = util::concat_vectors<GstElement*>(elements, dsp->getPartialPipeline());
        elements = util::concat_vectors<GstElement*>(elements, sink->getPartialPipeline());

        /* Add to bin */
        for (size_t i = 0; i < elements.size(); i++) {
            auto element = elements.at(i);
            auto next    = (i + 1) < elements.size() ? elements.at(i + 1) : NULL;

            /* Assert that both elements are a gst-object.
             * The next element is allowed to be NULL if the end of the pipeline was reached */
            assert(GST_IS_OBJECT(element));
            assert(GST_IS_OBJECT(next) || next == NULL);

            /* Add element to pipeline */
            gst_bin_add(GST_BIN (pipeline), element);

            /* Check if element requires stream conversion */
            if(next != NULL){
                if(!GstElementProperties(element).hasCompatibleCapsWith(next)){
                    util::debug("[" + std::string(GST_OBJECT_NAME(element)) +  "] Incompatible caps with " + GST_OBJECT_NAME(next) + ". Inserting 'audioconvert' element...");

                    GstElement* converter = gst_element_factory_make("audioconvert",
                                                                     (std::string(GST_OBJECT_NAME(element)) + "_audioconvert_" + std::to_string(util::random_number(INT_MAX))).c_str());
                    /* Add conversion element to pipeline */
                    gst_bin_add(GST_BIN(pipeline), converter);

                    elements.insert(elements.begin() + i + 1, converter);
                    i++;
                }
            }
        }

        /* Begin to link by dynamically detecting pad types and constellations of the elements */
        for (size_t i = 0; i < elements.size(); i++) {
            /* Load current and element to be linked into memory */
            auto element = elements.at(i);
            auto next    = (i + 1) < elements.size() ? elements.at(i + 1) : NULL;

            /* Assert that both elements are a gst-object.
             * The next element is allowed to be NULL is the end of the pipeline was reached */
            assert(GST_IS_OBJECT(element));
            assert(GST_IS_OBJECT(next) || next == NULL);

            /* Print basic pad information */
            auto props = GstElementProperties(element);
            props.printPadsOfElement(next);

            /* Proceed if the end of the pipeline hasn't been reached yet */
            if(next != NULL){

                /* Check if element still requires stream conversion */
                if(!GstElementProperties(element).hasCompatibleCapsWith(next)){
                    g_critical("[%s] Incompatible caps with %s. Cannot link. 'audioconvert' insertion appears to have failed",
                               GST_OBJECT_NAME(element), GST_OBJECT_NAME(next));
                }
                /* Are only SOMETIMES pads available on both sides? */
                if(props.isSometimesPadRequired(GST_PAD_SRC) && props.isSometimesPadRequired(GST_PAD_SINK)){
                    g_critical("[%s] Unable to link to %s. Both sides have only SOMETIMES pads",
                               GST_OBJECT_NAME(element), GST_OBJECT_NAME(next));
                    continue;
                }
                /* Is one of the pads neither ALWAYS or SOMETIMES? */
                else if (props.hasOnlyUnsupportedPads(GST_PAD_SRC) || props.hasOnlyUnsupportedPads(GST_PAD_SINK)){
                    g_critical("[%s] Unable to link to %s, no supported pads found", GST_OBJECT_NAME(element), GST_OBJECT_NAME(next));
                    continue;
                }
                /* Does the SRC pad require a dynamic link? */
                else if(props.isSometimesPadRequired(GST_PAD_SRC)){
                    g_info("[%s] No SRC ALWAYS pads detected; found SOMETIMES pad instead", GST_OBJECT_NAME(element));
                    g_signal_connect (element, "pad-added", G_CALLBACK (on_src_pad_added), next);
                }
                /* Does the SINK pad require a dynamic link? */
                else if(props.isSometimesPadRequired(GST_PAD_SINK)){
                    g_info("[%s] No SINK ALWAYS pads detected; found SOMETIMES pad instead", GST_OBJECT_NAME(next));
                    g_signal_connect (next, "pad-added", G_CALLBACK (on_sink_pad_added), element);
                }
                /* We're probably trying to link two ALWAYS pads. Link statically. */
                else {
                    if(!gst_element_link (element, next)){
                        g_critical("[%s] Failed to link statically, no alternatives detected. Check the dot log.", GST_OBJECT_NAME(element));
                        continue;
                    }
                }
            }

        }
    }

    /* Test: Save dot file for debugging */
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
}

void PulsePipelineManager::runLoop(){
    g_main_loop_run (loop);
}

void PulsePipelineManager::terminateLoop()
{
    g_main_loop_quit(loop);
}

GstElement *PulsePipelineManager::getPipeline() const
{
    return pipeline;
}


/**************
 *  Callbacks *
 **************/
static void on_src_pad_added (GstElement *element,
                              GstPad     *pad,
                              gpointer    data)
{
    GstPad *sinkpad;
    GstElement *nextElement = (GstElement *) data;

    assert(G_IS_OBJECT(nextElement));

    /* We can now link this pad with the sink pad */
    sinkpad = gst_element_get_static_pad (nextElement, "sink");

    assert(G_IS_OBJECT(sinkpad));

    util::debug(std::string(GST_OBJECT_NAME(element)) + " -> " + GST_OBJECT_NAME(nextElement) + ": dynamic source pad created (" + GST_OBJECT_NAME(pad) + " -> " + GST_OBJECT_NAME(sinkpad) + ")");

    GstPadLinkReturn result = gst_pad_link (pad, sinkpad);
    util::debug(" ~~> " + std::string(result == GST_PAD_LINK_OK ? "success" : "failed"));

    gst_object_unref (sinkpad);
}

static void on_sink_pad_added (GstElement *nextElement,
                               GstPad     *pad,
                               gpointer    data)
{
    GstPad *srcpad;
    GstElement *element = (GstElement *) data;

    assert(G_IS_OBJECT(element));

    /* We can now link this pad with the src pad */
    srcpad = gst_element_get_static_pad (element, "src");


    assert(G_IS_OBJECT(srcpad));

    util::debug(std::string(GST_OBJECT_NAME(element)) + " -> " + GST_OBJECT_NAME(nextElement) + ": dynamic sink pad created (" + GST_OBJECT_NAME(pad) + " -> " + GST_OBJECT_NAME(srcpad) + ")");

    GstPadLinkReturn result = gst_pad_link (srcpad, pad);
    util::debug(" ~~> " + std::string(result == GST_PAD_LINK_OK ? "success" : "failed"));

    gst_object_unref (srcpad);
}

#undef LOGTAG
