#pragma once

#include <audio_effects/effect_equalizer.h>

#include "Biquad.h"
#include "Effect.h"

#define CUSTOM_EQ_PARAM_LOUDNESS_CORRECTION 1000

class EffectEqualizer : public Effect {
    private:
    float mBand[6];
    Biquad mFilterL[5], mFilterR[5];

    /* Automatic equalizer */
    float mLoudnessAdjustment;

    float mLoudnessL;
    float mLoudnessR;
    int32_t mNextUpdate;
    int32_t mNextUpdateInterval;
    int64_t mPowerSquaredL;
    int64_t mPowerSquaredR;

    /* Smooth enable/disable */
    int32_t mFade;

    void setBand(int32_t idx, float dB);
    float getAdjustedBand(int32_t idx, float loudness);
    void refreshBands();
    void updateLoudnessEstimate(float& loudness, int64_t powerSquared);

    public:
    EffectEqualizer();
    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
