#pragma once

#include <stdint.h>
#include "hardware/system/audio.h"
#include "hardware/audio_effect.h"

#define NUMCHANNEL 2
class Effect
{
protected:
    bool mEnable;
    double mSamplingRate;
	int formatFloatModeInt32Mode;
    int32_t configure(void *pCmdData, effect_buffer_access_e* mAccessMode);

public:
    Effect();
    virtual ~Effect();
    virtual int32_t process(audio_buffer_t *in, audio_buffer_t *out) = 0;
    virtual int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData) = 0;
};