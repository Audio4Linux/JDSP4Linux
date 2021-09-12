#ifndef PULSESINKELEMENT_H
#define PULSESINKELEMENT_H

#include <string>
#include <glib.h>
#include <gst/gst.h>

#include "SinkElement.h"

class PulseSinkElement : public SinkElement
{
public:
    PulseSinkElement();
    ~PulseSinkElement();

    std::vector
    <GstElement*> getPartialPipeline();
    GstPad*       getSinkPad();
    GstElement*   getGstElement();
    SinkType      getType();

private:
    GstElement*   sink;
};

#endif // PULSESINKELEMENT_H
