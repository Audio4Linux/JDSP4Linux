#pragma once

#include "effect_cbassboost.h"

#include "Biquad.h"
#include "Effect.h"

class EffectBassBoost : public Effect {
    private:
    int16_t mStrength;
    int16_t mFilterType;
    float mCenterFrequency;
    Biquad mBoost;

    void refreshStrength();

    public:
    EffectBassBoost();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
