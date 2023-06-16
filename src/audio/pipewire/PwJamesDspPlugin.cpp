#include "PwJamesDspPlugin.h"

PwJamesDspPlugin::PwJamesDspPlugin(PwPipelineManager* pipe_manager)
    : PwPluginBase("@PwJamesDspPlugin: ", "JamesDsp", pipe_manager)
{
    this->dsp = (JamesDSPLib*) malloc(sizeof(JamesDSPLib));
    memset(this->dsp, 0, sizeof(JamesDSPLib));

    JamesDSPGlobalMemoryAllocation();
    JamesDSPInit(this->dsp, 128, 48000);

    _host = new DspHost(this->dsp, [this](DspHost::Message msg, std::any value){
        switch(msg)
        {
            case DspHost::SwitchPassthrough:
                bypass = !std::any_cast<bool>(value);
                break;
            default:
                // Redirect to parent handler
                _msgHandler(msg, value);
                break;
        }
    });
}

PwJamesDspPlugin::~PwJamesDspPlugin() {
  if (connected_to_pw) {
    disconnect_from_pw();
  }

  JamesDSPFree(this->dsp);
  JamesDSPGlobalMemoryDeallocation();

  util::debug(log_tag + name + " destroyed");
}

void PwJamesDspPlugin::setup() {
    JamesDSPSetSampleRate(this->dsp, rate, 0);
}

void PwJamesDspPlugin::process(float* left_in,
                               float* right_in,
                               float* left_out,
                               float* right_out,
                               size_t length)
{
  if (bypass)
  {
      memcpy(left_out, left_in, length * sizeof (float));
      memcpy(right_out, right_in, length * sizeof (float));
      return;
  }

  this->dsp->processFloatDeinterleaved(this->dsp, left_in, right_in, left_out, right_out, length);

  if (post_messages) {
    get_peaks(left_in, right_in, left_out, right_out, length);

    if (send_notifications) {
      notify();
    }
  }
}

DspStatus PwJamesDspPlugin::status()
{
    DspStatus status;
    status.AudioFormat = "32-bit floating point samples, little endian";
    status.SamplingRate = std::to_string(rate);
    status.IsProcessing = !bypass;
    return status;
}
