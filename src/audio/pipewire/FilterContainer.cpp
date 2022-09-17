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

#include "FilterContainer.h"

#include <thread>
#include <config/AppConfig.h>

FilterContainer::FilterContainer(PwPipelineManager* pipe_manager, PwPluginBase* plugin, AppConfig* settings) :
    pm(pipe_manager),
    settings(settings),
    plugin(plugin)
{

    // TODO remove
    /*if (settings->get<bool>(AppConfig::AudioOutputUseDefault)) {
        settings->set(AppConfig::AudioOutputDevice, QString::fromStdString(pm->output_device.name));
    } else {
        auto found = false;

        const auto output_device = settings->get<std::string>(AppConfig::AudioOutputDevice);

        for (const auto& [id, node] : pm->node_map) {
            if (node.name == output_device) {
                pm->output_device = node;

                found = true;

                break;
            }
        }

        if (!found) {
            settings->set(AppConfig::AudioOutputDevice, QString::fromStdString(pm->output_device.name));
        }
    }*/

    auto* PULSE_SINK = std::getenv("PULSE_SINK");
    if (PULSE_SINK != nullptr && PULSE_SINK != tags::pipewire::ee_sink_name) {
      for (const auto& [serial, node] : pm->node_map) {
        if (node.name == PULSE_SINK) {
          pm->output_device = node;

          settings->set(AppConfig::AudioOutputDevice, QString::fromStdString(pm->output_device.name));

          break;
        }
      }
    }

    pm->stream_output_added.connect(sigc::mem_fun(*this, &FilterContainer::on_app_added));
    pm->link_changed.connect(sigc::mem_fun(*this, &FilterContainer::on_link_changed));

    connect_filters();

    QObject::connect(settings, &AppConfig::updated, [&, this](const auto& key, const auto& value) {
        if(key != AppConfig::AudioOutputDevice)
            return;

        const auto name = value.toString().toStdString();

        if (name.empty()) {
            return;
        }

        for (const auto& [serial, node] : pm->node_map) {
            if (node.name == name) {
                pm->output_device = node;

                set_bypass(false);
                break;
            }
        }


    });

//------
     pm->sink_added.connect([=, this](const NodeInfo node) {
       if (node.name == settings->get<std::string>(AppConfig::AudioOutputDevice)) {
         pm->output_device = node;

         /*if (g_settings_get_boolean(global_settings, "bypass") != 0) {
           g_settings_set_boolean(global_settings, "bypass", 0);

           return;  // filter connected through update_bypass_state
         }*/

         util::debug("Target output device added: " + node.name);
         set_bypass(false);
       }
     });

     pm->sink_removed.connect([=, this](const NodeInfo node) {

       if (!settings->get<bool>(AppConfig::AudioOutputUseDefault)) {
         if (node.name == settings->get<std::string>(AppConfig::AudioOutputDevice)) {
           pm->output_device.id = SPA_ID_INVALID;
           pm->output_device.serial = SPA_ID_INVALID;
           util::debug("Target output device removed; fallback disabled");
         }
       }
     });
}

FilterContainer::~FilterContainer() {
    disconnect_filters();

    util::debug("destroyed");
}

void FilterContainer::on_app_added(const NodeInfo node_info) {
    const auto& is_blocklisted = settings->isAppBlocked(QString::fromStdString(node_info.name)); // TODO application_id or name?

    if (!is_blocklisted) {
        pm->connect_stream_output(node_info.id);
    }
}

auto FilterContainer::apps_want_to_play() -> bool {
  for (const auto& link : pm->list_links) {
    if (link.input_node_id == pm->ee_sink_node.id) {
      if (link.state == PW_LINK_STATE_ACTIVE) {
        return true;
      }
    }
  }

  return false;
}

void FilterContainer::on_link_changed(const LinkInfo link_info) {
  // We are not interested in the other link states

  if (link_info.state != PW_LINK_STATE_ACTIVE && link_info.state != PW_LINK_STATE_PAUSED) {
    return;
  }

  /*
    If bypass is enabled do not touch the plugin pipeline
  */

  if (bypass) {
    return;
  }

  if (apps_want_to_play()) {
    if (list_proxies.empty()) {
      util::debug("At least one app linked to our device wants to play. Linking our filters.");

      connect_filters();
    }
  } else {

    int inactivity_timeout = settings->get<int>(AppConfig::AudioInactivityTimeout);

    g_timeout_add_seconds(inactivity_timeout, GSourceFunc(+[](FilterContainer* self) {
                            if (!self->apps_want_to_play() && !self->list_proxies.empty()) {
                              util::debug("No app linked to our device wants to play. Unlinking our filters.");

                              self->disconnect_filters();
                            }

                            return G_SOURCE_REMOVE;
                          }),
                          this);
  }
}

void FilterContainer::connect_filters(const bool& bypass) {
  const auto output_device_name = settings->get<std::string>(AppConfig::AudioOutputDevice); // TODO test

  // checking if the output device exists
  if (output_device_name.empty()) {
    util::debug("No output device set. Aborting the link");

    return;
  }

  bool dev_exists = false;

  for (const auto& [serial, node] : pm->node_map) {
    if (node.name == output_device_name) {
      dev_exists = true;

      pm->output_device = node;

      break;
    }
  }

  if (!dev_exists) {
    util::debug("The output device " + output_device_name + " is not available. Aborting the link");

    return;
  }

  uint prev_node_id = pm->ee_sink_node.id;
  uint next_node_id = 0U;

  // link plugin
  if ((!plugin->connected_to_pw) ? plugin->connect_to_pw() : true) {
      next_node_id = plugin->get_node_id();

      const auto links = pm->link_nodes(prev_node_id, next_node_id);

      for (auto* link : links) {
        list_proxies.push_back(link);
      }

      if (links.size() == 2U) {
        prev_node_id = next_node_id;
      } else {
        util::warning(" link from node " + util::to_string(prev_node_id) + " to node " +
                      util::to_string(next_node_id) + " failed");
      }
  }

  // waiting for the output device ports information to be available.

  int timeout = 0;

  while (pm->count_node_ports(pm->output_device.id) < 2) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    timeout++;

    if (timeout > 10000) {  // 10 seconds
      util::warning("Information about the ports of the output device " + pm->output_device.name + " with id " +
                    util::to_string(pm->output_device.id) + " are taking to long to be available. Aborting the link");

      return;
    }
  }

  // link output device

   next_node_id = pm->output_device.id;

   const auto links = pm->link_nodes(prev_node_id, next_node_id);

   for (auto* link : links) {
     list_proxies.push_back(link);
   }

   if (links.size() < 2U) {
     util::warning(" link from node " + util::to_string(prev_node_id) + " to output device " +
                   util::to_string(next_node_id) + " failed");
   }
}

void FilterContainer::disconnect_filters() {
    std::set<uint> link_id_list;

    for (const auto& link : pm->list_links) {
      if (link.input_node_id == plugin->get_node_id() || link.output_node_id == plugin->get_node_id()) {
        link_id_list.insert(link.id);
      }
    }

    if (plugin->connected_to_pw) {
        util::debug("disconnecting the " + plugin->name + " filter from PipeWire");

        plugin->disconnect_from_pw();
    }

    for (const auto& id : link_id_list) {
      pm->destroy_object(static_cast<int>(id));
    }

    pm->destroy_links(list_proxies);

    list_proxies.clear();

    // remove_unused_filters();
}

void FilterContainer::set_bypass(const bool& state) {
  bypass = state;

  disconnect_filters();

  connect_filters(state);
}

void FilterContainer::activate_filters() {
    plugin->set_active(true);
}

void FilterContainer::deactivate_filters() {
    plugin->set_active(false);
}
