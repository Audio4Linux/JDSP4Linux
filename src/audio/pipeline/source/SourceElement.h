#ifndef SOURCEELEMENT_H
#define SOURCEELEMENT_H

#include <gst/gst.h>
#include "pipeline/BaseElement.h"

class SourceElement : public BaseElement
{
public:
    enum SrcType {
        File,
        Pulse,
    };

    virtual std::vector
    <GstElement*>       getPartialPipeline() = 0;
    virtual GstElement* getGstElement() = 0;
    virtual SrcType     getType() = 0;

    Type getBaseType(){
        return Type::SOURCE;
    };
};

#endif // SOURCEELEMENT_H
