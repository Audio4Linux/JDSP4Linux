#include "AutoSinkElement.h"

AutoSinkElement::AutoSinkElement()
{
    sink = gst_element_factory_make ("autoaudiosink", "autoaudiosink");
    gst_object_ref(sink);
}

AutoSinkElement::~AutoSinkElement()
{
    gst_object_unref(sink);
}

std::vector<GstElement*> AutoSinkElement::getPartialPipeline(){

    std::vector<GstElement*> pipe;
    pipe.push_back(sink);
    return pipe;
}


GstElement *AutoSinkElement::getGstElement()
{
    return sink;
}

GstPad *AutoSinkElement::getSinkPad()
{
    return gst_element_get_static_pad(sink, "sink");
}

SinkElement::SinkType AutoSinkElement::getType()
{
    return SinkType::Auto;
}


