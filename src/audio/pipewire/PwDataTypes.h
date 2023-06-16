#ifndef PWDATATYPES_H
#define PWDATATYPES_H

#include <pipewire/pipewire.h>
#include <memory>
#include <string>

struct NodeInfo {
  pw_proxy* proxy = nullptr;

  uint32_t id = SPA_ID_INVALID;

  uint32_t device_id = SPA_ID_INVALID;

  uint64_t serial = SPA_ID_INVALID;

  std::string name;

  std::string description;

  std::string media_class;

  std::string media_role;

  std::string app_icon_name;

  std::string media_icon_name;

  std::string device_icon_name;

  std::string media_name;

  std::string format;

  std::string application_id;

  int priority = -1;

  pw_node_state state = PW_NODE_STATE_IDLE;

  bool mute = false;

  bool connected = false;

  int n_input_ports = 0;

  int n_output_ports = 0;

  int rate = 1;  // used as divisor to calculate latency, so do not initialize it as 0

  uint32_t n_volume_channels = 0U;

  float latency = 0.0F;

  float volume = 0.0F;
};

struct LinkInfo {
  std::string path;

  uint32_t id = SPA_ID_INVALID;

  uint32_t input_node_id = 0U;

  uint32_t input_port_id = 0U;

  uint32_t output_node_id = 0U;

  uint32_t output_port_id = 0U;

  uint64_t serial = SPA_ID_INVALID;

  bool passive = false;  // does not cause the graph to be runnable

  pw_link_state state = PW_LINK_STATE_UNLINKED;
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

  uint32_t id = SPA_ID_INVALID;

  uint32_t node_id = 0U;

  uint32_t port_id = 0U;

  uint64_t serial = SPA_ID_INVALID;
};

struct ModuleInfo {
  uint32_t id;

  uint64_t serial = SPA_ID_INVALID;

  std::string name;

  std::string description;

  std::string filename;
};

struct ClientInfo {
  uint32_t id;

  uint64_t serial = SPA_ID_INVALID;

  std::string name;

  std::string access;

  std::string api;
};

struct DeviceInfo {
  uint32_t id;

  uint64_t serial = SPA_ID_INVALID;

  std::string name;

  std::string description;

  std::string nick;

  std::string media_class;

  std::string api;

  std::string input_route_name;

  std::string output_route_name;

  std::string bus_id;

  std::string bus_path;

  spa_param_availability input_route_available;

  spa_param_availability output_route_available;
};
#endif // PWDATATYPES_H
