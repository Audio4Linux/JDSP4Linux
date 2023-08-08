#ifndef JAMESDSPELEMENT_H
#define JAMESDSPELEMENT_H

#include "IDspElement.h"
#include "FilterElement.h"

class DspConfig;
class JamesDspElement : public FilterElement, public IDspElement
{
public:
    JamesDspElement();
    ~JamesDspElement();
    DspStatus status() override;

private:
    bool _state;

};

#endif // JAMESDSPELEMENT_H
