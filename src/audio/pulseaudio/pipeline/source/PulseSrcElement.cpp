#include "PulseSrcElement.h"

PulseSrcElement::PulseSrcElement()
{
    src = gst_element_factory_make ("pulsesrc", "pulsesrc");
    g_object_set(src, "volume", 1.0, nullptr);
    g_object_set(src, "mute", 0, nullptr);
    g_object_set(src, "provide-clock", 0, nullptr);
    g_object_set(src, "slave-method", 1, nullptr);  // re-timestamp
    g_object_set(src, "do-timestamp", 1, nullptr);
    gst_object_ref(src);

    queue = gst_element_factory_make ("queue", "src_queue");
    g_object_set(queue, "silent", 1, nullptr);
    g_object_set(queue, "flush-on-eos", 1, nullptr);
    g_object_set(queue, "max-size-buffers", 0, nullptr);
    g_object_set(queue, "max-size-bytes", 0, nullptr);
    g_object_set(queue, "max-size-time", 0, nullptr);
    gst_object_ref(queue);
}

PulseSrcElement::~PulseSrcElement()
{
    gst_object_unref(src);
    gst_object_unref(queue);
}

std::vector<GstElement*> PulseSrcElement::getPartialPipeline(){

    std::vector<GstElement*> pipe;
    pipe.push_back(src);
    pipe.push_back(queue);
    return pipe;
}

GstElement *PulseSrcElement::getGstElement()
{
    return src;
}

SourceElement::SrcType PulseSrcElement::getType()
{
    return SrcType::Pulse;
}
