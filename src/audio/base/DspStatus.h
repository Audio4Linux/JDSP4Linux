#ifndef DSPSTATUS_H
#define DSPSTATUS_H

#include <string>

typedef struct {
    std::string AudioFormat;
    int SamplingRate;
} DspStatus;

#endif // DSPSTATUS_H
