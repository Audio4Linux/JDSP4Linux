/*
 *  Copyright © 2017-2022 Wellington Wallace
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

#include <config/AppConfig.h>

FilterContainer::FilterContainer(PwPipelineManager* pipe_manager, PwPluginBase* plugin, AppConfig* settings) :
    log_tag("FilterContainer: "),
    pm(pipe_manager),
    settings(settings),
    plugin(plugin)
{
    pm->output_device = pm->default_output_device;

    if (settings->get<bool>(AppConfig::AudioOutputUseDefault)) {
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
    }

    auto* PULSE_SINK = std::getenv("PULSE_SINK");

    if (PULSE_SINK != nullptr) {
        for (const auto& [id, node] : pm->node_map) {
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

    QObject::connect(settings, &AppConfig::updated, nullptr, [&, this](const auto& key, const auto& value) {
        if(key != AppConfig::AudioOutputDevice)
            return;

        const auto name = value.toString().toStdString();

        if (name.empty()) {
            return;
        }

        for (const auto& [id, node] : pm->node_map) {
            if (node.name == name) {
                pm->output_device = node;

                disconnect_filters();

                connect_filters();

                break;
            }
        }
    });
}

FilterContainer::~FilterContainer() {
    disconnect_filters();

    util::debug(log_tag + "destroyed");
}

void FilterContainer::on_app_added(const uint id, const std::string name, const std::string media_class) {
    const auto& is_blocklisted = settings->isAppBlocked(QString::fromStdString(name));

    if (is_blocklisted) {
        pm->disconnect_stream_output(id, media_class);
    } else {
        pm->connect_stream_output(id, media_class);
    }
}

void FilterContainer::on_link_changed(LinkInfo link_info) {
    /*
    If bypass is enabled do not touch the plugin pipeline
   */

    if (bypass) {
        return;
    }

    auto want_to_play = false;

    for (const auto& link : pm->list_links) {
        if (link.input_node_id == pm->pe_sink_node.id) {
            if (link.state == PW_LINK_STATE_ACTIVE) {
                want_to_play = true;

                break;
            }
        }
    }

    if (!want_to_play) {
        disconnect_filters();

        return;
    }

    if (list_proxies.empty()) {
        connect_filters();
    }
}

void FilterContainer::connect_filters(const bool& bypass) {
    uint prev_node_id = pm->pe_sink_node.id;
    uint next_node_id = 0U;

    // link plugin
    if ((!plugin->connected_to_pw) ? plugin->connect_to_pw() : true) {
        next_node_id = plugin->get_node_id();

        const auto& links = pm->link_nodes(prev_node_id, next_node_id);

        const auto& link_size = links.size();

        for (size_t n = 0U; n < link_size; n++) {
            list_proxies.emplace_back(links[n]);
        }

        if (link_size == 2U) {
            prev_node_id = next_node_id;
        } else {
            util::warning(log_tag + " link from node " + std::to_string(prev_node_id) + " to node " +
                          std::to_string(next_node_id) + " failed");
        }
    }

    // link output device

    next_node_id = pm->output_device.id;

    const auto& links = pm->link_nodes(prev_node_id, next_node_id);

    const auto& link_size = links.size();

    for (size_t n = 0U; n < link_size; n++) {
        list_proxies.emplace_back(links[n]);
    }

    if (link_size < 2U) {
        util::warning(log_tag + " link from node " + std::to_string(prev_node_id) + " to output device " +
                      std::to_string(next_node_id) + " failed");
    }
}

void FilterContainer::disconnect_filters() {
    std::set<uint> list;

    for (const auto& link : pm->list_links) {
        if (link.input_node_id == plugin->get_node_id() || link.output_node_id == plugin->get_node_id()) {
            list.insert(link.id);
        }
    }

    for (const auto& id : list) {
        pm->destroy_object(static_cast<int>(id));
    }

    pm->destroy_links(list_proxies);

    list_proxies.clear();
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
