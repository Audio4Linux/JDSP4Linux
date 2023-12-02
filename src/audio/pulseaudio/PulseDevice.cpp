#include "PulseDevice.h"

PulseDevice::PulseDevice(const mySinkInfo &info)
{
    id = info.index;
    name = info.name;
    description = info.description;
    output_route_name = info.active_port;
    output_route_description = info.active_port;
}
