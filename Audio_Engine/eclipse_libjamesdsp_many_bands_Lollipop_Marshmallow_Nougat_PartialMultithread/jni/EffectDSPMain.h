#pragma once

#include "Effect.h"
#include "iir/Iir.h"
#include <pthread.h>
extern "C"
{
#include "bs2b.h"
#include "ArbFIRGen.h"
#include "compressor.h"
#include "gverb.h"
#include "reverb.h"
#include "AutoConvolver.h"
#include "valve/12ax7amp/Tube.h"
//#include "valve/wavechild670/wavechild670.h"
}

#define NUM_BANDS 10
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
	uint16_t currentframeCountInit;
	size_t memSize;
	// Float buffer
	float **inputBuffer, **outputBuffer, **finalImpulse, *tempImpulseFloat, **fullStereoBuf;
	// Incoming buffer from Java
	int32_t *tempImpulseInt32;
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
	// Equalizer
	Iir::Butterworth::LowShelf<4, Iir::DirectFormII> lsl;
	Iir::Butterworth::LowShelf<4, Iir::DirectFormII> lsr;
	Iir::Butterworth::BandShelf<4, Iir::DirectFormII> bs1l;
	Iir::Butterworth::BandShelf<4, Iir::DirectFormII> bs1r;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs2l;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs2r;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs3l;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs3r;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs4l;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs4r;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs5l;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs5r;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs6l;
	Iir::Butterworth::BandShelf<3, Iir::DirectFormII> bs6r;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs7l;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs7r;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs8l;
	Iir::Butterworth::BandShelf<2, Iir::DirectFormII> bs8r;
	Iir::Butterworth::HighShelf<3, Iir::DirectFormII> bs9l;
	Iir::Butterworth::HighShelf<3, Iir::DirectFormII> bs9r;
	// Variables
	float pregain, threshold, knee, ratio, attack, release;
	float finalGain, roomSize, fxreTime, damping, inBandwidth, earlyLv, tailLv, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType, bs2bLv;
	int16_t compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled, stereoWidenEnabled, convolverEnabled, convolverReady, bassLpReady, analogModelEnable, wavechild670Enabled, bs2bEnabled;
	int16_t samplesInc, impChannels, previousimpChannels;
	float tubedrive, tubebass, tubemid, tubetreble;
	float normalise;
	int32_t impulseLengthActual, convolverNeedRefresh;
	int16_t mPreset, mReverbMode;
	int isBenchData;
	double *benchmarkValue[2];
	void refreshBassLinearPhase(uint32_t actualframeCount, uint32_t tapsLPFIR, float bassBoostCentreFreq);
	int refreshConvolver(uint32_t actualframeCount);
	void refreshStereoWiden(uint32_t parameter);
	void refreshCompressor();
	void refreshEqBands(double *bands);
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
	inline void channel_split(int16_t* buffer, size_t num_frames, float** chan_buffers)
	{
		size_t i, samples = num_frames << 1;
		for (i = 0; i < samples; i++)
			chan_buffers[i % 2][i >> 1] = (float)((double)(buffer[i]) * 0.000030517578125);
	}
	inline void channel_join(float** chan_buffers, int16_t* buffer, size_t num_frames, float scalar)
	{
		size_t i, samples = num_frames << 1;
		for (i = 0; i < samples; i++)
			buffer[i] = (int16_t)(tanh(chan_buffers[i % 2][i >> 1] * finalGain * scalar) * 32768.0f);
	}
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
