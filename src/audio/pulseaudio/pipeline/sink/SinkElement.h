#ifndef SINKELEMENT_H
#define SINKELEMENT_H

#include <string>
#include <glib.h>
#include <gst/gst.h>
#include "pipeline/BaseElement.h"

class SinkElement : public BaseElement
{
public:
    enum SinkType {
        Auto,
        Pulse,
    };

    virtual std::vector
    <GstElement*>        getPartialPipeline() = 0;
    virtual GstElement*  getGstElement() = 0;
    virtual GstPad*      getSinkPad() = 0;
    virtual SinkType     getType() = 0;

    Type getBaseType(){
        return Type::SINK;
    };
};

#endif // SINKELEMENT_H
