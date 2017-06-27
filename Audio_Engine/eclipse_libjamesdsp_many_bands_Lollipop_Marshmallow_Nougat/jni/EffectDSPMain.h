#pragma once

#include "Effect.h"
#include "iir/Iir.h"
#include "compressor.h"
#include "gverb.h"
#include "reverb.h"
#include "libHybridConv.h"

#define NUM_BANDS 10
class EffectDSPMain : public Effect
{
protected:
	uint16_t oldframeCountInit;
	size_t memSize;
	// Float buffer
	float **inputBuffer, **outputBuffer, **finalImpulse, *tempImpulseFloat;
	// Incoming buffer from Java
	int32_t *tempImpulseInt32;
	// Effect units
	sf_compressor_state_st compressor;
	ty_gverb *verbL, *verbR;
	sf_reverb_state_st myreverb;
	HConvSingle *bassBoostLp, *convolver;
	int threadResult;
	// Bass boost
	Iir::Butterworth::LowShelf<4, Iir::DirectFormII> bbL;
	Iir::Butterworth::LowShelf<4, Iir::DirectFormII> bbR;
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
	Iir::Butterworth::HighShelf<4, Iir::DirectFormII> bs9l;
	Iir::Butterworth::HighShelf<4, Iir::DirectFormII> bs9r;
	// Variables
	double mBand[NUM_BANDS];
	float pregain, threshold, knee, ratio, attack, release;
	float bassBoostCentreFreq, finalGain, roomSize, fxreTime, damping, inBandwidth, earlyLv, tailLv, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType;
	int16_t compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled, stereoWidenEnabled, normaliseEnabled, clipMode, convolverEnabled, convolverReady, bassLpReady;
	int16_t numTime2Send, samplesInc, impChannels, previousimpChannels;
	float normalise;
	int32_t impulseLengthActual, convolverNeedRefresh;
	int16_t mPreset, mReverbMode;
	int tapsLPFIR;
	void normaliseToLevel(float* buffer, size_t num_samps, float level);
	void refreshBassLinearPhase(uint32_t actualframeCount);
	void refreshConvolver(uint32_t actualframeCount);
	void refreshStereoWiden(uint32_t parameter);
	void refreshCompressor();
	void refreshEqBands();
	void refreshReverb();
	void refreshBass();
	inline void channel_split(int16_t* buffer, size_t num_frames, float** chan_buffers)
	{
		size_t i;
		size_t samples = num_frames << 1;
		for (i = 0; i < samples; i++)
			chan_buffers[(i % 2)][i >> 1] = (float)((double)(buffer[i]) * 0.000030517578125);
	}
	inline void channel_join(float** chan_buffers, int16_t* buffer, size_t num_frames)
	{
		size_t i;
		size_t samples = num_frames << 1;
		if (!clipMode)
		{
			for (i = 0; i < samples; i++)
			{
				buffer[i] = chan_buffers[i % 2][i >> 1] * finalGain;
				if (buffer[i] > 32767)
					buffer[i] = 32767;
				if (buffer[i] < -32768)
					buffer[i] = -32768;
			}
		}
		else
		{
			for (i = 0; i < samples; i++)
				buffer[i] = tanh(chan_buffers[i % 2][i >> 1]) * finalGain;
		}
	}
	float fmax(float a,double b)
	{
		return (b<a)?a:b;
	}
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
