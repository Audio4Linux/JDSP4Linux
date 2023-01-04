#ifndef PULSEMANAGER_H
#define PULSEMANAGER_H

#include <glib.h>
#include <glibmm.h>
#include <pulse/pulseaudio.h>
#include <pulse/thread-mainloop.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>

#include "PulseDataTypes.h"

#define SINK_NAME "JamesDSP"
#define SINK_DESC "JamesDSP"

/*
 * Note: Code derived from PulseEffects
 * https://github.com/wwmm/pulseeffects
 */

class ParseAppInfo;

class PulseManager {

public:
    PulseManager();
    PulseManager(const PulseManager&) = delete;
    auto operator=(const PulseManager&) -> PulseManager& = delete;
    PulseManager(const PulseManager&&) = delete;
    auto operator=(const PulseManager &&) -> PulseManager& = delete;
    ~PulseManager();

    pa_threaded_mainloop* main_loop = nullptr;

    myServerInfo server_info;
    std::shared_ptr<mySinkInfo> apps_sink_info;

    auto get_sink_info(const std::string& name) -> std::shared_ptr<mySinkInfo>;

    std::vector<std::string> blacklist_in;   // for input effects
    std::vector<std::string> blacklist_out;  // for output effects

    void find_sink_inputs();
    void find_source_outputs();
    void find_sinks();
    auto move_sink_input_to_gstmgr(const std::string& name, uint32_t idx) -> bool;
    auto remove_sink_input_from_gstmgr(const std::string& name, uint32_t idx) -> bool;
    void set_sink_input_volume(const std::string& name, uint32_t idx, uint8_t channels, uint32_t value);
    void set_sink_input_mute(const std::string& name, uint32_t idx, bool state);
    void set_source_output_volume(const std::string& name, uint32_t idx, uint8_t channels, uint32_t value);
    void set_source_output_mute(const std::string& name, uint32_t idx, bool state);
    void get_sink_input_info(uint32_t idx);
    void update_server_info(const pa_server_info* info);
    void get_modules_info();
    void get_clients_info();

    sigc::signal<void, std::shared_ptr<mySinkInfo>> sink_added;
    sigc::signal<void, std::shared_ptr<mySinkInfo>> sink_changed;
    sigc::signal<void, uint32_t> sink_removed;
    sigc::signal<void, std::string> new_default_sink;
    sigc::signal<void, std::string> new_default_source;
    sigc::signal<void, std::shared_ptr<AppInfo>> sink_input_added;
    sigc::signal<void, std::shared_ptr<AppInfo>> sink_input_changed;
    sigc::signal<void, uint32_t> sink_input_removed;
    sigc::signal<void, std::shared_ptr<AppInfo>> source_output_added;
    sigc::signal<void, std::shared_ptr<AppInfo>> source_output_changed;
    sigc::signal<void, uint32_t> source_output_removed;
    sigc::signal<void> server_changed;
    sigc::signal<void, std::shared_ptr<myModuleInfo>> module_info;
    sigc::signal<void, std::shared_ptr<myClientInfo>> client_info;

private:
    bool context_ready = false;

    pa_mainloop_api* main_loop_api = nullptr;
    pa_context* context = nullptr;

    std::array<std::string, 22> blacklist_apps = {"JamesDSP", "jamesdsp",
        "GstEffectManager", "jdsp-gui", "PulseEffectsWebrtcProbe", "gsd-media-keys",
        "GNOME Shell", "libcanberra", "Screenshot", "speech-dispatcher", "gst-launch-1.0",
        "EasyEffects", "easyeffects", "easyeffects_soe", "easyeffects_sie", "EasyEffectsWebrtcProbe", "pavucontrol",
        "PulseAudio Volume Control", "speech-dispatcher-dummy", "Mutter","gameoverlayui"};

    std::array<std::string, 2> blacklist_media_role = {"event", "Notification"};


    std::array<std::string, 4> blacklist_media_name = {"pulsesink probe", "bell-window-system", "audio-volume-change",
                                                       "screen-capture"};

    std::array<std::string, 4> blacklist_app_id = {"com.github.wwmm.pulseeffects.sinkinputs",
                                                   "com.github.wwmm.pulseeffects.sourceoutputs",
                                                   "org.PulseAudio.pavucontrol", "org.gnome.VolumeControl"};

    static void context_state_cb(pa_context* ctx, void* data);

    void subscribe_to_events();

    void get_server_info();

    auto get_default_sink_info() -> std::shared_ptr<mySinkInfo>;

    auto load_sink(const std::string& name, const std::string& description, uint32_t rate) -> std::shared_ptr<mySinkInfo>;

    void load_apps_sink();

    auto load_module(const std::string& name, const std::string& argument) -> bool;

    void unload_module(uint32_t idx);

    void unload_sinks();

    void drain_context();

    void new_app(const pa_sink_input_info* info);

    void new_app(const pa_source_output_info* info);

    void changed_app(const pa_sink_input_info* info);

    void changed_app(const pa_source_output_info* info);

    static void print_app_info(const std::shared_ptr<AppInfo>& info);

    auto app_is_connected(const pa_sink_input_info* info) -> bool;

    auto app_is_connected(const pa_source_output_info* info) -> bool;

    static auto get_latency(const pa_sink_input_info* info) -> uint32_t { return info->sink_usec; }

    static auto get_latency(const pa_source_output_info* info) -> uint32_t { return info->source_usec; }

    template <typename T>
    auto parse_app_info(const T& info) -> std::shared_ptr<AppInfo> {
        std::string app_name;
        std::string media_name;
        std::string media_role;
        std::string app_id;
        auto ai = std::make_shared<AppInfo>();
        bool forbidden_app = false;

        auto prop = pa_proplist_gets(info->proplist, "application.name");

        if (prop != nullptr) {
            app_name = prop;

            forbidden_app =
                    std::find(std::begin(blacklist_apps), std::end(blacklist_apps), app_name) != std::end(blacklist_apps);

            if (forbidden_app) {
                return nullptr;
            }
        }

        prop = pa_proplist_gets(info->proplist, "media.name");

        if (prop != nullptr) {
            media_name = prop;

            if (app_name.empty()) {
                app_name = media_name;
            }

            forbidden_app = std::find(std::begin(blacklist_media_name), std::end(blacklist_media_name), media_name) !=
                    std::end(blacklist_media_name);

            if (forbidden_app) {
                return nullptr;
            }
        }

        prop = pa_proplist_gets(info->proplist, "media.role");

        if (prop != nullptr) {
            media_role = prop;

            forbidden_app = std::find(std::begin(blacklist_media_role), std::end(blacklist_media_role), media_role) !=
                    std::end(blacklist_media_role);

            if (forbidden_app) {
                return nullptr;
            }
        }

        prop = pa_proplist_gets(info->proplist, "application.id");

        if (prop != nullptr) {
            app_id = prop;

            forbidden_app =
                    std::find(std::begin(blacklist_app_id), std::end(blacklist_app_id), app_id) != std::end(blacklist_app_id);

            if (forbidden_app) {
                return nullptr;
            }
        }

        prop = pa_proplist_gets(info->proplist, "application.icon_name");

        std::string icon_name;

        if (prop != nullptr) {
            icon_name = prop;
        } else {
            prop = pa_proplist_gets(info->proplist, "media.icon_name");

            if (prop != nullptr) {
                if (std::strcmp(prop, "audio-card-bluetooth") ==
                        0) {  // there is no GTK icon with this name given by Pulseaudio =/
                } else {
                    icon_name = "bluetooth-symbolic";
                }
            } else {
                icon_name = "audio-x-generic-symbolic";
            }
        }

        // Connection flag: it specifies only the primary state that can be enabled/disabled by the user
        ai->connected = app_is_connected(info);

        // Initialize visibility to true, it will be properly updated forward
        ai->visible = true;

        // linear volume
        ai->volume = 100 * pa_cvolume_max(&info->volume) / PA_VOLUME_NORM;

        if (info->resample_method) {
            ai->resampler = info->resample_method;
        } else {
            ai->resampler = "none";
        }

        ai->format = pa_sample_format_to_string(info->sample_spec.format);

        ai->index = info->index;
        ai->name = app_name;
        ai->app_type = "";
        ai->icon_name = icon_name;
        ai->channels = info->volume.channels;
        ai->rate = info->sample_spec.rate;
        ai->mute = info->mute;
        ai->buffer = info->buffer_usec;
        ai->latency = get_latency(info);
        ai->corked = info->corked;
        ai->wants_to_play = ai->connected && !ai->corked;

        return ai;
    }
};

#endif //PULSEMANAGER_H
