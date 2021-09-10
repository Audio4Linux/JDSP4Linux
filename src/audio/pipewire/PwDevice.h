#ifndef PWDEVICE_H
#define PWDEVICE_H

#include "PwPipelineManager.h"
#include <IOutputDevice.h>

class PwDevice : public IOutputDevice
{
public:
    PwDevice(NodeInfo info);

};

#endif // PWDEVICE_H
