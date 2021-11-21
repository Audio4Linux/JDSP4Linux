#ifndef PWDATATYPES_H
#define PWDATATYPES_H

#include <pipewire/pipewire.h>
#include <memory>
#include <string>

struct NodeInfo {
  pw_proxy* proxy = nullptr;

  uint id = SPA_ID_INVALID;

  uint device_id = SPA_ID_INVALID;

  std::string name;

  std::string description;

  std::string media_class;

  std::string app_icon_name;

  std::string media_icon_name;

  std::string device_icon_name;

  std::string media_name;

  std::string format;

  int priority = -1;

  pw_node_state state;

  bool mute = false;

  int n_input_ports = 0;

  int n_output_ports = 0;

  uint rate = 0U;

  int n_volume_channels = 0;

  float latency = 0.0F;

  float volume = 0.0F;
};

struct LinkInfo {
  std::string path;

  uint id = SPA_ID_INVALID;

  uint input_node_id = 0U;

  uint input_port_id = 0U;

  uint output_node_id = 0U;

  uint output_port_id = 0U;

  bool passive = false;  // does not cause the graph to be runnable

  pw_link_state state;
};

struct PortInfo {
  std::string path;

  std::string format_dsp;

  std::string audio_channel;

  std::string name;

  std::string direction;

  bool physical = false;

  bool terminal = false;

  bool monitor = false;

  uint id = SPA_ID_INVALID;

  uint node_id = 0U;

  uint port_id = 0U;
};

struct ModuleInfo {
  uint id;

  std::string name;

  std::string description;

  std::string filename;
};

struct ClientInfo {
  uint id;

  std::string name;

  std::string access;

  std::string api;
};

struct DeviceInfo {
  uint id;

  std::string name;

  std::string description;

  std::string nick;

  std::string media_class;

  std::string api;

  std::string input_route_name;

  std::string output_route_name;

  spa_param_availability input_route_available;

  spa_param_availability output_route_available;
};
#endif // PWDATATYPES_H
