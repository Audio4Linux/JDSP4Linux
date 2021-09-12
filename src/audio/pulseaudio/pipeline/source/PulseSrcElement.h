#ifndef PULSESRCELEMENT_H
#define PULSESRCELEMENT_H

#include <string>
#include <glib.h>
#include <gst/gst.h>

#include "SourceElement.h"

class PulseSrcElement : public SourceElement
{
public:
    PulseSrcElement();
    ~PulseSrcElement();

    std::vector
    <GstElement*> getPartialPipeline();
    SrcType       getType();
    GstElement*   getGstElement();

private:
    GstElement*   src;
    GstElement*   queue;
};


#endif // PULSESRCELEMENT_H
