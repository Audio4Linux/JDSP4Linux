#pragma once

#include "Effect.h"
#include <pthread.h>
extern "C"
{
#include "bs2b.h"
#include "mnspline.h"
#include "ArbFIRGen.h"
#include "compressor.h"
#include "gverb.h"
#include "reverb.h"
#include "AutoConvolver.h"
#include "valve/12ax7amp/Tube.h"
//#include "valve/wavechild670/wavechild670.h"
}

#define NUM_BANDS 15
#define NUM_BANDSM1 NUM_BANDS-1
class EffectDSPMain : public Effect
{
protected:
	typedef struct threadParamsConv {
		AutoConvolverMono **conv;
		float **in, **out;
		size_t frameCount;
	} ptrThreadParamsFullStConv;
	typedef struct threadParamsTube {
		tubeFilter *tube;
		float **in;
		size_t frameCount;
	} ptrThreadParamsTube;
	static void *threadingConvF(void *args);
	static void *threadingConvF1(void *args);
	static void *threadingConvF2(void *args);
	static void *threadingTube(void *args);
	ptrThreadParamsFullStConv fullStconvparams, fullStconvparams1;
	ptrThreadParamsTube rightparams2;
	pthread_t rightconv, rightconv1, righttube;
	int DSPbufferLength, inOutRWPosition;
	size_t memSize;
	// Float buffer
	float *inputBuffer[2], *outputBuffer[2], *tempBuf[2], **finalImpulse, *tempImpulseFloat, *tempImpulseIncoming;
	// Fade ramp
	float ramp;
	// Effect units
	sf_compressor_state_st compressor;
	ty_gverb *verbL, *verbR;
	sf_reverb_state_st myreverb;
	AutoConvolverMono **bassBoostLp;
	AutoConvolverMono **convolver, **fullStereoConvolver;
	tubeFilter tubeP[2];
	t_bs2bdp bs2b;
//	Wavechild670 *compressor670;
	int threadResult;
	ArbitraryEq *arbEq;
	float *xaxis, *yaxis;
	int eqfilterLength;
	AutoConvolverMono **FIREq;
	// Variables
	float pregain, threshold, knee, ratio, attack, release, tubedrive, normalise;
	float finalGain, roomSize, fxreTime, damping, inBandwidth, earlyLv, tailLv, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType, eqFilterType, bs2bLv, compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled,
	stereoWidenEnabled, convolverEnabled, convolverReady, bassLpReady, eqFIRReady, analogModelEnable, wavechild670Enabled, bs2bEnabled, pamssEnabled;
	int16_t samplesInc, impChannels, previousimpChannels;
	int32_t impulseLengthActual, convolverNeedRefresh;
	int16_t mPreset, mReverbMode;
	int isBenchData;
	double *benchmarkValue[2];
	void FreeBassBoost();
	void FreeEq();
	void FreeConvolver();
	void channel_splitFloat(const float *buffer, unsigned int num_frames, float **chan_buffers, unsigned int num_channels);
	void refreshTubeAmp();
	void refreshBassLinearPhase(uint32_t actualframeCount, uint32_t tapsLPFIR, float bassBoostCentreFreq);
	int refreshConvolver(uint32_t actualframeCount);
	void refreshStereoWiden(uint32_t parameter);
	void refreshCompressor();
	void refreshEqBands(uint32_t actualframeCount, float *bands);
	void refreshReverb();
	inline float normaliseToLevel(float* buffer, size_t num_frames, float level)
	{
		int i;
		float max = 0, amp;
		for (i = 0; i < num_frames; i++)
			max = fabsf(buffer[i]) < max ? max : fabsf(buffer[i]);
		amp = level / max;
		for (i = 0; i < num_frames; i++)
			buffer[i] *= amp;
		return max;
	}
	inline float map(float x, float in_min, float in_max, float out_min, float out_max)
	{
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
