#ifndef DSPHOST_H
#define DSPHOST_H

#include <functional>

#include "config/DspConfig.h"

extern "C" {
#include <jdsp_header.h>
}

class DspHost
{
public:
    typedef std::function<void(DspConfig::Key,QVariant)> HandleExtraConfigFunc;

    DspHost(void* dspPtr, HandleExtraConfigFunc&& extraHandler);

    bool update(DspConfig* config);
    void reloadLiveprog(DspConfig* config = nullptr);

private:
    HandleExtraConfigFunc _extraFunc;
    JamesDSPLib* _dsp;
    DspConfig* _cache;
    std::function<void(DspConfig::Key)> _extraHandler;

    void updateLimiter(DspConfig *config);
    void updateFirEqualizer(DspConfig *config);
    void updateVdc(DspConfig *config);
    void updateCompressor(DspConfig *config);
    void updateReverb(DspConfig *config);
    void updateConvolver(DspConfig *config);
    void updateGraphicEq(DspConfig *config);
    void updateCrossfeed(DspConfig *config);
};

#endif // DSPHOST_H
