#pragma once

#include "effect_cstereowide.h"

#include "Biquad4proc.h"
#include "Effect.h"

class EffectStereoWide : public Effect {
    private:
    int16_t mStrength;
    // Matrix M (center channel) coefficient
    double mMatrixMCoeff;
    // Matrix S (side channel) coefficient
    double mMatrixSCoeff;

    void refreshStrength();

    public:
    EffectStereoWide();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
