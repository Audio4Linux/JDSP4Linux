#include "PulseSinkElement.h"

PulseSinkElement::PulseSinkElement()
{
    sink = gst_element_factory_make ("pulsesink", "pulsesink");
    g_object_set(sink, "volume", 1.0, nullptr);
    g_object_set(sink, "mute", 0, nullptr);
    g_object_set(sink, "provide-clock", 1, nullptr);

    gst_object_ref(sink);
}

PulseSinkElement::~PulseSinkElement()
{
    gst_object_unref(sink);
}

std::vector<GstElement *> PulseSinkElement::getPartialPipeline()
{
    std::vector<GstElement*> pipe;
    pipe.push_back(sink);
    return pipe;
}

GstPad *PulseSinkElement::getSinkPad()
{
    return gst_element_get_static_pad(sink, "sink");
}

GstElement *PulseSinkElement::getGstElement()
{
    return sink;
}

SinkElement::SinkType PulseSinkElement::getType()
{
    return SinkType::Pulse;
}
