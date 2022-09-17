/*
 *  Note: The code in this file was adopted from EasyEffects (https://github.com/wwmm/easyeffects)
 *  This version includes minor changes that enable compatibility with JamesDSP.
 *
 *  The original copyright notice is attached below.
 */

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

#ifndef FILTERCONTAINER_H
#define FILTERCONTAINER_H

#include "PwPipelineManager.h"
#include "PwBasePlugin.h"

class AppConfig;

class FilterContainer {
 public:
  FilterContainer(PwPipelineManager* pipe_manager, PwPluginBase* plugin, AppConfig* settings);
  FilterContainer(const FilterContainer&) = delete;
  auto operator=(const FilterContainer&) -> FilterContainer& = delete;
  FilterContainer(const FilterContainer&&) = delete;
  auto operator=(const FilterContainer&&) -> FilterContainer& = delete;
  ~FilterContainer();

  void set_bypass(const bool& state);

  void connect_filters(const bool& bypass = false);

  void disconnect_filters();

  PwPipelineManager* pm = nullptr;

  AppConfig* settings;

 private:
  bool bypass = false;

  PwPluginBase* plugin;

  std::vector<pw_proxy*> list_proxies, list_proxies_listen_mic;

  void activate_filters();

  void deactivate_filters();

  void on_app_added(NodeInfo node_info);

  void on_link_changed(LinkInfo link_info);

  auto apps_want_to_play() -> bool;

};

#endif
