#ifndef DSPHOST_H
#define DSPHOST_H

#include <functional>
#include <any>
#include <list>

#include "EelVariable.h"

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
    std::list<EelVariable> enumEelVariables();
    void dispatch(Message msg, std::any value);

    JamesDSPLib *dsp() const;

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

inline int32_t arySearch(int32_t *array, int32_t N, int32_t x)
{
    for (int32_t i = 0; i < N; i++)
    {
        if (array[i] == x)
            return i;
    }
    return -1;
}
#define FLOIDX 20000
inline void* GetStringForIndex(eel_string_context_state *st, float val, int32_t write)
{
    int32_t castedValue = (int32_t)(val + 0.5f);
    if (castedValue < FLOIDX)
        return 0;
    int32_t idx = arySearch(st->map, st->slot, castedValue);
    if (idx < 0)
        return 0;
    if (!write)
    {
        s_str *tmp = &st->m_literal_strings[idx];
        const char *s = s_str_c_str(tmp);
        return (void*)s;
    }
    else
        return (void*)&st->m_literal_strings[idx];
}

#endif // DSPHOST_H
