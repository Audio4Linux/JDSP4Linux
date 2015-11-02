#pragma once

#include "effect_camplifier.h"

#include "Effect.h"

class EffectAmplifier : public Effect {
    private:
    int16_t mStrength;

    public:
    EffectAmplifier();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
