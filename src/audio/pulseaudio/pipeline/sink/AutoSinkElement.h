#ifndef FILESINKELEMENT_H
#define FILESINKELEMENT_H

#include <string>
#include <glib.h>
#include <gst/gst.h>

#include "SinkElement.h"

class AutoSinkElement : public SinkElement
{
public:
    AutoSinkElement();
    ~AutoSinkElement();

    std::vector
    <GstElement*> getPartialPipeline();
    GstElement*   getGstElement();
    GstPad*       getSinkPad();
    SinkType      getType();

private:
    GstElement*   sink;
};

#endif // FILESINKELEMENT_H
