#pragma once

#include "effect_cvirtualizer.h"

#include "Biquad4proc.h"
#include "Delay.h"
#include "Effect.h"
#include "bandpass.h"
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
    ne10_int32_t* left;
    ne10_int32_t* right;
    ne10_int32_t* imp;
    ne10_fft_cpx_int32_t *cpximp;
    ne10_fft_r2c_cfg_int32_t cfgimp;
    ne10_fft_cpx_int32_t *cpxMul;
    ne10_fft_cpx_int32_t *cpxleft;
    ne10_fft_cpx_int32_t *cpxright;
    ne10_fft_r2c_cfg_int32_t cfgFor;
    ne10_fft_r2c_cfg_int32_t cfgInv;
    int16_t powTwoFrame;
    int16_t impSize;
    int16_t convoledSize;
    int16_t cpxMultiplicationSize;

    void refreshStrength();
    void printTest();
    int next_power_of_two(float a_F);

    public:
    EffectVirtualizer();
    ~EffectVirtualizer();

    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
