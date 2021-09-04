#ifndef GENERICFILTERELEMENT_H
#define GENERICFILTERELEMENT_H

#include <map>
#include <string>
#include <variant>
#include <functional>
#include <gst/gst.h>

#include "BaseElement.h"
#include "Utils.h"

typedef struct std::function<void(GstBuffer*, gpointer)> BufferProbeDataCallback;
typedef struct {
    BufferProbeDataCallback callback;
    gpointer user_data;
} BufferProbeData;

class FilterElement : public BaseElement
{
public:
    FilterElement(const std::string& _plugin_name, const std::string& _package_name);
    ~FilterElement();

    std::vector<GstElement*> getPartialPipeline();
    GstElement*     getGstElement();
    std::string     getFactoryName() const;

    ulong           attachProbe(GstPadProbeType type, GstPadProbeCallback cb, gpointer user_data = NULL);
    ulong           attachBufferProbe(BufferProbeDataCallback callback, gpointer user_data = NULL);
    void            removeProbe(ulong id);

    Type            getBaseType() { return Type::FILTER; };

private:
    GstElement* filter;
    std::string factory_name;
    std::string baseplugin_name;
    std::map<ulong, BufferProbeData*> probebuffer_data;
};

#endif // GENERICFILTERELEMENT_H
