#include "PwDevice.h"

PwDevice::PwDevice(NodeInfo info)
{
    id = info.id;
    name = info.name;
    description = info.description;
}

PwDevice::PwDevice(DeviceInfo info)
{
    id = info.id;
    name = info.name;
    description = info.description;
    output_route_name = info.output_route_name;
    output_route_description = info.output_route_desc;

    QList<std::string> usedKeys;
    for (const auto& route : info.output_routes) {
        if(!usedKeys.contains(route.name)) {
            output_routes.append(Route(route.name, route.description));
            usedKeys.append(route.name);
        }
    }
}
