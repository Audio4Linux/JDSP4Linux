#ifndef PWJAMESDSPPLUGIN_H
#define PWJAMESDSPPLUGIN_H

#include "PwBasePlugin.h"
#include "IDspElement.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
extern "C" {
#include "jdsp_header.h"
}
#pragma GCC diagnostic pop


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
