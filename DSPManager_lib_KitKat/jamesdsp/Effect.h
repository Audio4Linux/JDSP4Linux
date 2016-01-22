#pragma once

#include <stdint.h>
#include <system/audio.h>
#include <hardware/audio_effect.h>

class Effect {
    private:
    effect_buffer_access_e mAccessMode;

    protected:
    bool mEnable;
    float mSamplingRate;

    /* AudioFlinger only gives us 16-bit PCM for now. */
    inline int32_t read(audio_buffer_t *in, int32_t idx) {
        return in->s16[idx] << 8;
    }

    /* AudioFlinger only expects 16-bit PCM for now. */
    inline void write(audio_buffer_t *out, int32_t idx, int32_t sample) {
        if (mAccessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE) {
            sample += out->s16[idx] << 8;
        }
        sample = (sample) >> 8;
        if (sample > 32767) {
            sample = 32767;
        }
        if (sample < -32768) {
            sample = -32768;
        }
        out->s16[idx] = sample;
    }

    int32_t configure(void *pCmdData);

    public:
    Effect();
    virtual ~Effect();
    virtual int32_t process(audio_buffer_t *in, audio_buffer_t *out) = 0;
    virtual int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData) = 0;
};
