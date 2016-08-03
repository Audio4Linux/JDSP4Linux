#pragma once

#include "effect_cbassboost.h"

#include "Biquad.h"
#include "Effect.h"

class EffectBassBoost : public Effect {
    private:
    int16_t mStrength, mStrengthK;
    int16_t mFilterType;
    int32_t noiseon;
    float mCenterFrequency;
    Biquad mStage1L, mStage1R, mBoostL, mBoostR;

    protected:
    uint8_t mPreviousRandom;

    /* High-passed triangular probability density function.
     * Output varies from -0xff to 0xff. */
    inline int32_t triangularDither8() {
        uint8_t newRandom = prng();
        int32_t rnd = int32_t(mPreviousRandom) - int32_t(newRandom);
        mPreviousRandom = newRandom;
        return rnd;
    }
    public:
    EffectBassBoost();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
