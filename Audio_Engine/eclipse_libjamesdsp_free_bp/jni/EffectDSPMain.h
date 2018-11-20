#pragma once

#include "Effect.h"
#include <pthread.h>
#include <math.h>
extern "C"
{
#include "bs2b.h"
#include "mnspline.h"
#include "ArbFIRGen.h"
#include "vdc.h"
#include "compressor.h"
#include "reverb.h"
#include "AutoConvolver.h"
#include "valve/12ax7amp/Tube.h"
#include "JLimiter.h"
//#include "valve/wavechild670/wavechild670.h"
}

#define NUM_BANDS 15
#define NUM_BANDSM1 NUM_BANDS-1
class EffectDSPMain : public Effect
{
protected:
	int stringLength;
	char *stringEq;
	DirectForm2 **df441, **df48, **dfResampled, **sosPointer;
	int sosCount, resampledSOSCount, usedSOSCount;
	typedef struct threadParamsConv {
		AutoConvolver1x1 **conv;
		double **in, **out;
		size_t frameCount;
	} ptrThreadParamsFullStConv;
	typedef struct threadParamsTube {
		tubeFilter *tube;
		double **in;
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
	// double buffer
	double *inputBuffer[2], *outputBuffer[2], *tempBuf[2], **finalImpulse, *tempImpulsedouble;
	float *tempImpulseIncoming;
	// Fade ramp
	double ramp;
	// Effect units
	JLimiter kLimiter;
	sf_compressor_state_st compressor;
	sf_reverb_state_st myreverb;
	AutoConvolver1x1 **bassBoostLp;
	AutoConvolver1x1 **convolver, **fullStereoConvolver;
	tubeFilter tubeP[2];
	t_bs2bdp bs2b;
//	Wavechild670 *compressor670;
	ArbitraryEq *arbEq;
	double *xaxis, *yaxis;
	int eqfilterLength;
	AutoConvolver1x1 **FIREq;
	// Variables
	double pregain, threshold, knee, ratio, attack, release, tubedrive, bassBoostCentreFreq, convGaindB, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType, eqFilterType, bs2bLv, compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled,
	stereoWidenEnabled, convolverEnabled, convolverReady, bassLpReady, eqFIRReady, analogModelEnable, bs2bEnabled, viperddcEnabled;
	int16_t mPreset, samplesInc, stringIndex, impChannels, previousimpChannels;
	int32_t impulseLengthActual, convolverNeedRefresh;
	int isBenchData;
	double *benchmarkValue[2];
	void FreeBassBoost();
	void FreeEq();
	void FreeConvolver();
	void channel_splitFloat(const float *buffer, unsigned int num_frames, float **chan_buffers, unsigned int num_channels);
	void refreshTubeAmp();
	void refreshBassLinearPhase(uint32_t actualframeCount, uint32_t tapsLPFIR, double bassBoostCentreFreq);
	int refreshConvolver(uint32_t actualframeCount);
	void refreshStereoWiden(uint32_t parameter);
	void refreshCompressor();
	void refreshEqBands(uint32_t actualframeCount, double *bands);
	void refreshReverb();
	inline double map(double x, double in_min, double in_max, double out_min, double out_max)
	{
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
