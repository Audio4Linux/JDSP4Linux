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

#ifndef PWJAMESDSPPLUGIN_H
#define PWJAMESDSPPLUGIN_H

#include "PwBasePlugin.h"
#include "IDspElement.h"

extern "C" {
#include "jdsp_header.h"
}

class PwJamesDspPlugin : public PwPluginBase, public IDspElement {
public:
  PwJamesDspPlugin(PwPipelineManager* pipe_manager);
  PwJamesDspPlugin(const PwJamesDspPlugin&) = delete;
  auto operator=(const PwJamesDspPlugin&) -> PwJamesDspPlugin& = delete;
  PwJamesDspPlugin(const PwJamesDspPlugin&&) = delete;
  auto operator=(const PwJamesDspPlugin&&) -> PwJamesDspPlugin& = delete;
  ~PwJamesDspPlugin() override;

  void setup() override;

  void process(float* left_in,
               float* right_in,
               float* left_out,
               float* right_out,
               size_t length) override;

  DspStatus status() override;

   JamesDSPLib* dsp;
};

#endif
