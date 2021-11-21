#ifndef JAMESDSPELEMENT_H
#define JAMESDSPELEMENT_H

#include "IDspElement.h"
#include "FilterElement.h"

extern "C" {
#include <jdsp_header.h>
}

class DspConfig;
class JamesDspElement : public FilterElement, public IDspElement
{
public:
    JamesDspElement();
    DspStatus status() override;

private:
    bool _state;

};

#endif // JAMESDSPELEMENT_H
