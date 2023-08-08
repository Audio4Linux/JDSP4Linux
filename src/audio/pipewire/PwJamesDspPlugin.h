#ifndef PWJAMESDSPPLUGIN_H
#define PWJAMESDSPPLUGIN_H

#include "PwBasePlugin.h"
#include "IDspElement.h"
#include "IAudioService.h"

extern "C" {
#include "jdsp_header.h"
}

class PwJamesDspPlugin : public PwPluginBase, public IDspElement {
public:
  PwJamesDspPlugin(PwPipelineManager* pipe_manager, IAudioService* parent_service);
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
