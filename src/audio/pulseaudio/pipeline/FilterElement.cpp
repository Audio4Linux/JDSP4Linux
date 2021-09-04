#include "FilterElement.h"

#include <async++.h>

GstPadProbeReturn on_buffer_probe_cb ([[maybe_unused]] GstPad * pad,
GstPadProbeInfo * info,
gpointer user_data){

    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);
    BufferProbeData* data = (BufferProbeData*)user_data;

    if(!buffer){
        g_warning("Buffer probe: buffer is NULL");
        return GST_PAD_PROBE_OK;
    }

    /* Push refcount and handle it via the signal */
    buffer = gst_buffer_ref(buffer);
    async::spawn([data, buffer] {

        data->callback(buffer, data->user_data);
    });

    return GST_PAD_PROBE_OK;
}


FilterElement::FilterElement(const std::string& _plugin_name, const std::string& _package_name)
{
    factory_name = _plugin_name;
    baseplugin_name = _package_name;
    filter = gst_element_factory_make(_plugin_name.c_str(), NULL);
    gst_object_ref(filter);
}

FilterElement::~FilterElement()
{
    gst_object_unref(filter);
}

ulong FilterElement::attachProbe(GstPadProbeType type,
                                 GstPadProbeCallback cb,
                                 gpointer user_data){

    GstPad* pad = gst_element_get_static_pad (filter, "sink");

    if(!pad){
        g_warning("FilterElement: Failed to attach probe. No static sink pad found.");
        return 0;
    }

    ulong id = gst_pad_add_probe (pad, type, cb, user_data, NULL);
    if(id == 0){
        g_warning("FilterElement: Failed to attach probe. No probe pending, id is zero.");
    }

    gst_object_unref (pad);
    return id;
}

ulong FilterElement::attachBufferProbe(BufferProbeDataCallback callback,
                                       gpointer user_data){
    BufferProbeData* data = new BufferProbeData;
    data->user_data = user_data;
    data->callback = callback;
    ulong id = attachProbe(GST_PAD_PROBE_TYPE_BUFFER, on_buffer_probe_cb, data);

    if(id == 0)
        delete data;
    else
        probebuffer_data[id] = data;

    return id;
}


void FilterElement::removeProbe(ulong id){
    GstPad* pad = gst_element_get_static_pad (filter, "sink");

    if(!pad){
        g_warning("FilterElement: Failed to remove probe. No static sink pad found.");
        return;
    }
    if(probebuffer_data.count(id))
        delete probebuffer_data[id];

    gst_pad_remove_probe(pad, id);
    gst_object_unref (pad);
}

std::string FilterElement::getFactoryName() const
{
    return factory_name;
}

std::vector<GstElement*> FilterElement::getPartialPipeline(){

    std::vector<GstElement*> pipe;
    pipe.push_back(filter);
    return pipe;
}

GstElement *FilterElement::getGstElement()
{
    return filter;
}
