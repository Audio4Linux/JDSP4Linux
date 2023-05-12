#ifndef IOUTPUTDEVICE_H
#define IOUTPUTDEVICE_H

#include <stdint.h>
#include <string>

class IOutputDevice
{
public:
    IOutputDevice(){}
    IOutputDevice(std::string name, std::string description) : name(name), description(description){}

    uint32_t id = ((uint32_t)0xffffffff);

    std::string name;

    std::string description;
};

#endif // IOUTPUTDEVICE_H
