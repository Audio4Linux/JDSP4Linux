#ifndef DSPHOST_H
#define DSPHOST_H

#include <functional>
#include <any>
#include <list>
#include <memory>

#include "EelVariable.h"
#include "Utils.h"

class DspConfig;
class QVariant;

class DspHost
{
public:
    enum Message {
        SwitchPassthrough,
        EelCompilerStart,
        EelCompilerResult,
        EelWriteOutputBuffer,
        ConvolverInfoChanged,
        PrintfWriteOutputBuffer
    };

    typedef std::function<void(Message,std::any)> MessageHandlerFunc;

    DspHost(void* dspPtr, MessageHandlerFunc&& extraHandler);
    ~DspHost();

    bool update(DspConfig* config, bool ignoreCache = false);
    void updateFromCache();
    void reloadLiveprog(DspConfig* config = nullptr);
    std::vector<EelVariable> enumEelVariables();
    bool manipulateEelVariable(const char *name, float value);
    void freezeLiveprogExecution(bool freeze);
    void dispatch(Message msg, std::any value);

private:
    MessageHandlerFunc _extraFunc;

    /* Workaround: typedef structs cannot be forward declared >:(
       Including jdsp_header.h here directly extremely pollutes the main code
       due to #defines and other global definitons that may conflict */
    void* _dsp; // JamesDSPLib*
    DspConfig* _cache;

    void updateLimiter(DspConfig *config);
    void updateFirEqualizer(DspConfig *config);
    void updateVdc(DspConfig *config);
    void updateCompander(DspConfig *config);
    void updateReverb(DspConfig *config);
    void updateConvolver(DspConfig *config);
    void updateGraphicEq(DspConfig *config);
    void updateCrossfeed(DspConfig *config);

};

/* C interop function */
static void receiveLiveprogStdOut(const char* buffer, void* userData);

#endif // DSPHOST_H
