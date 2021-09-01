#ifndef JAMESDSPELEMENT_H
#define JAMESDSPELEMENT_H

#include "FilterElement.h"

extern "C" {
#include <jdsp_header.h>
}

class DspConfig;
class JamesDspElement : public FilterElement
{
public:
    JamesDspElement();
    bool update(DspConfig* config);

private:
    JamesDSPLib* _dsp;
    DspConfig* _cache;

    void updateLimiter(DspConfig *config);
    void updateFirEqualizer(DspConfig *config);
    void updateVdc(DspConfig *config);
    void updateCompressor(DspConfig *config);
    void updateReverb(DspConfig *config);
    void updateConvolver(DspConfig *config);
};

#endif // JAMESDSPELEMENT_H
