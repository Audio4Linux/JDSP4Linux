#pragma once

#include "effect_cvirtualizer.h"

#include "Biquad4proc.h"
#include "Delay.h"
#include "Effect.h"
#include "dsp/NE10.h"
class EffectVirtualizer : public Effect {
    private:
    int16_t mStrength;
    size_t audioBufferSize;
    float mEchoDecay;
    int16_t mReverbMode, mReverbPreset;
    bool mDeep, mWide;
    int64_t mLevel;
    inline double* gen1DArray(int arraySize) {
    	double* array1D;
    	array1D = (double*)malloc(arraySize * sizeof(double*));
    	return array1D;
    }
    Delay mReverbDelayL, mReverbDelayR;
    int64_t mDelayDataL, mDelayDataR;
    Biquad4proc mLocalization;

    void refreshStrength();
    void printTest();

    public:
    EffectVirtualizer();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
