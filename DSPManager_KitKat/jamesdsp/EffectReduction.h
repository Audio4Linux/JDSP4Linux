#pragma once

#include "effect_creduction.h"

#include "Biquad.h"
#include "Effect.h"

class EffectReduction : public Effect {
    private:
    int16_t mStrength;
    float mHighCenterFrequency;
    Biquad mBoostHigh;

    void refreshStrength();
    void refreshtwentyStrength();

    public:
    EffectReduction();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
