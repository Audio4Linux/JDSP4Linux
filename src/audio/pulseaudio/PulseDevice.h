#ifndef PULSEDEVICE_H
#define PULSEDEVICE_H

#include "PulseDataTypes.h"

#include <IOutputDevice.h>

class PulseDevice : public IOutputDevice
{
public:
    PulseDevice(const mySinkInfo& info);
};

#endif // PULSEDEVICE_H
