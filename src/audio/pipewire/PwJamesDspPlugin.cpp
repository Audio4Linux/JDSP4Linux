#include "PwJamesDspPlugin.h"
#include <QString>

extern "C" {
#ifdef DEBUG_FPE
#include <fenv.h>
#endif
#include <PrintfStdOutExtension.h>
}

void receivePrintfStdout(const char* msg, void* userdata) {
    auto* plugin = static_cast<PwJamesDspPlugin*>(userdata);
    if(plugin != nullptr) {
        plugin->callMessageHandler(DspHost::PrintfWriteOutputBuffer, QString(msg));
    }
}

PwJamesDspPlugin::PwJamesDspPlugin(PwPipelineManager* pipe_manager, IAudioService* parent_service)
    : PwPluginBase("@PwJamesDspPlugin: ", "JamesDsp", pipe_manager)
{
    setMessageHandler(std::bind(&IAudioService::handleMessage, parent_service, std::placeholders::_1, std::placeholders::_2));
    setPrintfStdOutHandler(receivePrintfStdout, this);

    this->dsp = (JamesDSPLib*) malloc(sizeof(JamesDSPLib));
    memset(this->dsp, 0, sizeof(JamesDSPLib));

#ifdef DEBUG_FPE
    feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT & ~FE_INVALID);
#endif
    JamesDSPGlobalMemoryAllocation();
    JamesDSPInit(this->dsp, 128, 48000);
#ifdef DEBUG_FPE
    fedisableexcept(FE_ALL_EXCEPT & ~FE_INEXACT & ~FE_INVALID);
#endif
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


#ifdef DEBUG_FPE
  feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
#endif
  JamesDSPFree(this->dsp);
  JamesDSPGlobalMemoryDeallocation();
#ifdef DEBUG_FPE
  fedisableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);
#endif

  setPrintfStdOutHandler(nullptr, nullptr);
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

#ifdef DEBUG_FPE
  feenableexcept(FE_DIVBYZERO);
#endif
  this->dsp->processFloatDeinterleaved(this->dsp, left_in, right_in, left_out, right_out, length);

  if (post_messages) {
    get_peaks(left_in, right_in, left_out, right_out, length);

    if (send_notifications) {
      notify();
    }
  }
#ifdef DEBUG_FPE
  fedisableexcept(FE_DIVBYZERO);
#endif
}

DspStatus PwJamesDspPlugin::status()
{
    DspStatus status;
    status.AudioFormat = "32-bit floating point samples, little endian";
    status.SamplingRate = std::to_string(rate);
    status.IsProcessing = !bypass;
    return status;
}
