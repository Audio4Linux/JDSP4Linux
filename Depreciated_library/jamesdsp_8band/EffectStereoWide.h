#pragma once

#include "effect_cstereowide.h"

#include "Biquad.h"
#include "Delay.h"
#include "Effect.h"
#include "FIR16.h"

/**
 * Effect that enhances stereo wideness
 * Based off OpenSLES Stereo Widener and ideas from Waves S1 Stereo Imager DSP plugin
 * See the CPP file for a detailed explanation of the implementation
 */

class EffectStereoWide : public Effect {
    private:
    int16_t mStrength;

    Delay mSlightDelay;
    int64_t mDelayData;
    Biquad mHighPass;
    Biquad mBassTrim;

    // Matrix M (center channel) coefficient
    double mMatrixMCoeff;
    // Matrix S (side channel) coefficient
    double mMatrixSCoeff;
    // Split EQ HighPass on S coefficient
    double mSplitEQCoeff;
    // Split EQ HighPass compensation on M coefficient
    double mSplitEQCompCoeff;
    // Bass trim coefficient
    double mBassTrimCoeff;
    float mFineTuneFreq;

    void refreshStrength();

    public:
    EffectStereoWide();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
