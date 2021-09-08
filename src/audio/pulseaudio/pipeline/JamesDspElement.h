#ifndef JAMESDSPELEMENT_H
#define JAMESDSPELEMENT_H

#include "FilterElement.h"
#include "IDspElement.h"

extern "C" {
#include <jdsp_header.h>
}

class DspConfig;
class JamesDspElement : public FilterElement, public IDspElement
{
public:
    JamesDspElement();
    DspStatus status() override;
};

#endif // JAMESDSPELEMENT_H
