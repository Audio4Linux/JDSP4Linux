#include "PulseDevice.h"

PulseDevice::PulseDevice(const mySinkInfo &info)
{
    id = info.index;
    name = info.name;
    description = info.description;
}
