#pragma once

#include "effect_cvirtualizer.h"

#include "Biquad4proc.h"
#include "Delay.h"
#include "Effect.h"
#include "FIR16.h"

class EffectVirtualizer : public Effect {
    private:
    int16_t mStrength;
    float mEchoDecay;
    bool mDeep, mWide;
    int64_t mLevel;

    Delay mReverbDelayL, mReverbDelayR;
    int64_t mDelayDataL, mDelayDataR;
    Biquad4proc mLocalization;

    void refreshStrength();

    public:
    EffectVirtualizer();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
