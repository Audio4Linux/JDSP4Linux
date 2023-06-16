#ifndef PWPIPELINEMANAGER_H
#define PWPIPELINEMANAGER_H

/*
 *  Note: The code in this file was adopted from EasyEffects (https://github.com/wwmm/easyeffects)
 *  This version includes minor changes that enable compatibility with JamesDSP.
 *
 *  The original copyright notice is attached below.
 */

/*
 *  Copyright © 2017-2023 Wellington Wallace
 *
 *  This file is part of EasyEffects.
 *
 *  EasyEffects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EasyEffects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EasyEffects.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <glibmm.h>
#include <pipewire/extensions/metadata.h>
#include <pipewire/pipewire.h>
#include <sigc++/sigc++.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/utils/json.h>
#include <spa/utils/result.h>
#include <spa/param/audio/type-info.h>
#include <spa/monitor/device.h>
#include <spa/utils/keys.h>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <map>
#include <span>
#include <ranges>

#include "PwDataTypes.h"

class PwPipelineManager {
 public:
  PwPipelineManager();
  PwPipelineManager(const PwPipelineManager&) = delete;
  auto operator=(const PwPipelineManager&) -> PwPipelineManager& = delete;
  PwPipelineManager(const PwPipelineManager&&) = delete;
  auto operator=(const PwPipelineManager&&) -> PwPipelineManager& = delete;
  ~PwPipelineManager();

  pw_thread_loop* thread_loop = nullptr;
  pw_core* core = nullptr;
  pw_registry* registry = nullptr;
  pw_metadata* metadata = nullptr;

  inline static bool exiting = false;

  inline static bool exclude_monitor_stream = true;

  spa_hook metadata_listener{};

  std::map<uint64_t, NodeInfo> node_map;

  std::vector<LinkInfo> list_links;

  std::vector<PortInfo> list_ports;

  std::vector<ModuleInfo> list_modules;

  std::vector<ClientInfo> list_clients;

  std::vector<DeviceInfo> list_devices;

  std::string default_output_device_name, default_input_device_name;

  NodeInfo ee_sink_node, ee_source_node;
  NodeInfo output_device, input_device;

  constexpr static auto blocklist_node_name =   {"Easy Effects",
                                                 "EasyEffects",
                                                 "easyeffects",
                                                 "easyeffects_soe",
                                                 "easyeffects_sie",
                                                 "EasyEffectsWebrtcProbe",
                                                 "pavucontrol",
                                                 "PulseAudio Volume Control",
                                                 "libcanberra",
                                                 "gsd-media-keys",
                                                 "GNOME Shell",
                                                 "speech-dispatcher-espeak-ng",
                                                 "speech-dispatcher",
                                                 "speech-dispatcher-dummy",
                                                 "Mutter",
                                                 "gameoverlayui",
                                                 "JamesDSP",
                                                 "jamesdsp",
                                                 "GstEffectManager",
                                                 "jdsp-gui",
                                                 "PulseEffectsWebrtcProbe",
                                                 "Screenshot",
                                                 "gst-launch-1.0"};

  std::array<std::string, 2U> blocklist_app_id = {"org.PulseAudio.pavucontrol", "org.gnome.VolumeControl"}; // <- TODO double-check

  std::array<std::string, 2U> blocklist_media_role = {"event", "Notification"};

  std::string header_version, library_version, core_name, version;
  std::string default_clock_rate = "0";
  std::string default_min_quantum = "0";
  std::string default_max_quantum = "0";
  std::string default_quantum = "0";

  auto node_map_at_id(const uint32_t& id) -> NodeInfo&;

  auto stream_is_connected(const uint32_t& id, const std::string& media_class) -> bool;

  void connect_stream_output(const uint32_t& id) const;

  void connect_stream_input(const uint32_t& id) const;

  void disconnect_stream(const uint32_t& id) const;

  static void set_node_volume(pw_proxy* proxy, const uint32_t& n_vol_ch, const float& value);

  static void set_node_mute(pw_proxy* proxy, const bool& state);

  auto count_node_ports(const uint32_t& node_id) -> uint32_t;

  /*
    Links the output ports of the node output_node_id to the input ports of the node input_node_id
  */

  auto link_nodes(const uint32_t& output_node_id,
                  const uint32_t& input_node_id,
                  const bool& probe_link = false,
                  const bool& link_passive = true) -> std::vector<pw_proxy*>;

  void destroy_object(const int& id) const;

  /*
    Destroy all the filters links
  */

  void destroy_links(const std::vector<pw_proxy*>& list) const;

  void lock() const;

  void unlock() const;

  void sync_wait_unlock() const;

  auto wait_full() const -> int;

  static void lock_node_map();

  static void unlock_node_map();

  static auto json_object_find(const char* obj, const char* key, char* value, const size_t& len) -> int;

  sigc::signal<void(const NodeInfo)> stream_output_added;
  sigc::signal<void(const NodeInfo)> stream_input_added;
  sigc::signal<void(const NodeInfo)> stream_output_changed;
  sigc::signal<void(const NodeInfo)> stream_input_changed;
  sigc::signal<void(const uint64_t)> stream_output_removed;
  sigc::signal<void(const uint64_t)> stream_input_removed;

  /*
    Do not pass NodeInfo by reference. Sometimes it dies before we use it and a segmentation fault happens.
  */

  sigc::signal<void(const NodeInfo)> source_added;
  sigc::signal<void(const NodeInfo)> source_changed;
  sigc::signal<void(const NodeInfo)> source_removed;
  sigc::signal<void(const NodeInfo)> sink_added;
  sigc::signal<void(const NodeInfo)> sink_changed;
  sigc::signal<void(const NodeInfo)> sink_removed;
  sigc::signal<void(const std::string)> new_default_sink_name;
  sigc::signal<void(const std::string)> new_default_source_name;
  sigc::signal<void(const DeviceInfo)> device_input_route_changed;
  sigc::signal<void(const DeviceInfo)> device_output_route_changed;

  sigc::signal<void(const LinkInfo)> link_changed;

 private:
  pw_context* context = nullptr;
  pw_proxy *proxy_stream_output_sink = nullptr, *proxy_stream_input_source = nullptr;

  spa_hook core_listener{}, registry_listener{};

  void set_metadata_target_node(const uint32_t& origin_id, const uint32_t& target_id, const uint64_t& target_serial) const;

};

namespace tags::pipewire {

inline constexpr auto ee_source_name = "jamesdsp_source";

inline constexpr auto ee_sink_name = "jamesdsp_sink";

}  // namespace tags::pipewire

namespace tags::pipewire::media_class {

inline constexpr auto device = "Audio/Device";

inline constexpr auto sink = "Audio/Sink";

inline constexpr auto source = "Audio/Source";

inline constexpr auto virtual_source = "Audio/Source/Virtual";

inline constexpr auto input_stream = "Stream/Input/Audio";

inline constexpr auto output_stream = "Stream/Output/Audio";

}  // namespace tags::pipewire::media_class


namespace tags::app {

inline constexpr auto id = "me.timschneeberger.JDSP4Linux";

}  // namespace tags::app
#endif
