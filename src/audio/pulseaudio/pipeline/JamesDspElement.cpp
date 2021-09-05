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

    _host = new DspHost(gDspPtr, [this](DspConfig::Key key, QVariant value){
        switch(key)
        {
            case DspConfig::master_enable:
                this->setValues("dsp_enable", value.toBool(), NULL);
                break;
            default:
                break;
        }
    });
}

DspStatus JamesDspElement::status()
{
    DspStatus status;
    const char* format = NULL;
    this->getValues("dsp_srate", &status.SamplingRate,
                    "dsp_format", &format, NULL);
    status.AudioFormat = format;

    return status;
}

