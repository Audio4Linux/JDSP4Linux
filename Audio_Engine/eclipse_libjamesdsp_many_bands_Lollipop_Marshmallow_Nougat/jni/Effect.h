#pragma once

#include <stdint.h>
#include "hardware/system/audio.h"
#include "hardware/audio_effect.h"

static inline uint8_t prng() {
    static uint32_t seed;
    seed = seed * 1664525 + 1013904223;
    return seed >> 22;
}
#define NUMCHANNEL 2
class Effect {
    private:
    effect_buffer_access_e mAccessMode;

    protected:
    bool mEnable;
    float mSamplingRate;
	
    int32_t configure(void *pCmdData, uint16_t* frameCountInit);

    public:
    Effect();
    virtual ~Effect();
    virtual int32_t process(audio_buffer_t *in, audio_buffer_t *out) = 0;
    virtual int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData) = 0;
};
