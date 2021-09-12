#include "config/DspConfig.h"

#include "JamesDspElement.h"

#include "DspHost.h"

JamesDspElement::JamesDspElement() : FilterElement("jamesdsp", "jamesdsp")
{
    gboolean gEnabled;
    gpointer gDspPtr = NULL;
    this->getValues("dsp_ptr", &gDspPtr,
                    "dsp_enable", &gEnabled, NULL);

    assert(!gEnabled); // check if underlying object is fresh

    _host = new DspHost(gDspPtr, [this](DspHost::Message msg, std::any value){
        switch(msg)
        {
            case DspHost::SwitchPassthrough:
                this->setValues("dsp_enable", std::any_cast<bool>(value), NULL);
                break;
            default:
                // Redirect to parent handler
                _msgHandler(msg, value);
                break;
        }
    });
}

DspStatus JamesDspElement::status()
{
    DspStatus status;
    const char* format = NULL;
    int srate = 0;
    bool enabled = false;
    this->getValues("dsp_srate", &srate,
                    "dsp_format", &format,
                    "dsp_enable", &enabled, NULL);

    if(srate == 0)
        status.SamplingRate = "Unknown";
    else
        status.SamplingRate = std::to_string(srate);

    status.AudioFormat = format;
    status.IsProcessing = enabled;

    return status;
}

