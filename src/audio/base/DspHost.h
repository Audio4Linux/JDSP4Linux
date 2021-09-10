#ifndef DSPHOST_H
#define DSPHOST_H

#include <functional>
#include <any>

extern "C" {
#include <jdsp_header.h>
}

class DspConfig;
class QVariant;

class DspHost
{
public:
    enum Message {
        SwitchPassthrough,
        EelCompilerStart,
        EelCompilerResult,
        EelWriteOutputBuffer
    };

    typedef std::function<void(Message,std::any)> MessageHandlerFunc;

    DspHost(void* dspPtr, MessageHandlerFunc&& extraHandler);
    ~DspHost();

    bool update(DspConfig* config, bool ignoreCache = false);
    void updateFromCache();
    void reloadLiveprog(DspConfig* config = nullptr);
    void dispatch(Message msg, std::any value);

private:
    MessageHandlerFunc _extraFunc;
    JamesDSPLib* _dsp;
    DspConfig* _cache;

    void updateLimiter(DspConfig *config);
    void updateFirEqualizer(DspConfig *config);
    void updateVdc(DspConfig *config);
    void updateCompressor(DspConfig *config);
    void updateReverb(DspConfig *config);
    void updateConvolver(DspConfig *config);
    void updateGraphicEq(DspConfig *config);
    void updateCrossfeed(DspConfig *config);
};

/* C interop function */
static void receiveLiveprogStdOut(const char* buffer, void* userData);

#endif // DSPHOST_H
