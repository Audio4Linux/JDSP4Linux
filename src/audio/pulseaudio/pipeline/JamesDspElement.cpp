#include "config/DspConfig.h"

#include "JamesDspElement.h"
#include "DspHost.h"

extern "C" {
#include <PrintfStdOutExtension.h>
}

void receivePrintfStdout(const char* msg, void* userdata) {
    auto* plugin = static_cast<JamesDspElement*>(userdata);
    if(plugin != nullptr) {
        plugin->callMessageHandler(DspHost::PrintfWriteOutputBuffer, QString(msg));
    }
}


JamesDspElement::JamesDspElement() : FilterElement("jamesdsp", "jamesdsp")
{
    setPrintfStdOutHandler(receivePrintfStdout, this);

    gboolean gEnabled;
    gpointer gDspPtr = NULL;
    this->getValues("dsp_ptr", &gDspPtr,
                    "dsp_enable", &gEnabled, NULL);

    assert(!gEnabled); // check if underlying object is fresh
    _state = gEnabled;

    _host = new DspHost(gDspPtr, [this](DspHost::Message msg, std::any value){
        switch(msg)
        {
            case DspHost::SwitchPassthrough:
                this->setValues("dsp_enable", std::any_cast<bool>(value), NULL);
                _state = std::any_cast<bool>(value);
                break;
            default:
                // Redirect to parent handler
                _msgHandler(msg, value);
                break;
        }
});
}

JamesDspElement::~JamesDspElement()
{
    setPrintfStdOutHandler(nullptr, nullptr);
}

DspStatus JamesDspElement::status()
{
    DspStatus status;
    char* format = NULL;
    char* srate = NULL;

    this->getValues("dsp_format", &format, NULL);
    this->getValues("dsp_srate", &srate, NULL);

    status.SamplingRate = srate;
    status.AudioFormat = format;
    status.IsProcessing = _state;

    return status;
}

