#ifndef DSPSTATUS_H
#define DSPSTATUS_H

#include <string>

typedef struct {
    std::string AudioFormat;
    std::string SamplingRate;
    bool IsProcessing;
} DspStatus;

#endif // DSPSTATUS_H
