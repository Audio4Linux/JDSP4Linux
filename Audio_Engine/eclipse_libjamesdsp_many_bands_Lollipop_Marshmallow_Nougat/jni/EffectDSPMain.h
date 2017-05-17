#pragma once

#include "Effect.h"
#include "iir/Iir.h"
#include "compressor.h"
#include "gverb.h"
#include "reverb.h"
#include "libHybridConv.h"

#define NUM_BANDS 10
class EffectDSPMain : public Effect {
private:
	// Float buffer
	float **inputBuffer, **outputBuffer, **finalImpulse, *tempImpulseFloat;
	// Incoming buffer from Java
	int32_t *tempImpulseInt32;
	// Effect units
	sf_compressor_state_st compressor;
	ty_gverb *verbL, *verbR;
	sf_reverb_state_st myreverb;
	HConvSingle *bassBosstLp, *convolver;
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
	float mBand[NUM_BANDS];
	float pregainCom, threshold, knee, ratio, attack, release, predelay, releasezone1, releasezone2, releasezone3, releasezone4, postgain;
	float bassBoostCentreFreq, finalGain, roomSize, fxreTime, damping, spread, deltaSpread, inBandwidth, earlyLv, tailLv, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType;
	int16_t compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled, stereoWidenEnabled, normaliseEnabled, clipMode, convolverEnabled, convolverReady, bassParameterChanged;
	int16_t numTime2Send, samplesInc, impChannels;
	int32_t impulseLengthActual, convolverNeedRefresh;
	int16_t mPreset, mReverbMode, widenStrength;
	int tapsLPFIR;
	void refreshCompressor();
	void refreshBass();
	void refreshBassLinearPhase(uint32_t actualframeCount);
	void refreshEqBands();
	void refreshReverb();
	void refreshStereoWiden();
	void refreshConvolver(uint32_t actualframeCount);
	void normalize(float* buffer, int num_samps, float maxval);
	inline void channel_split(int16_t* buffer, int num_frames, float** chan_buffers)
	{
		int i;
		int samples = num_frames * 2;
		for (i = 0; i < samples; i++)
			chan_buffers[(i % 2)][i / 2] = (float)((double)(buffer[i]) * 0.000030517578125);
	}
	inline void channel_join(float** chan_buffers, int16_t* buffer, int num_frames)
	{
		uint16_t i;
		int samples = num_frames * 2;
		if (!clipMode) {
			for (i = 0; i < samples; i++)
			{
				buffer[i] = chan_buffers[i % 2][i / 2] * finalGain;
				if (buffer[i] > 32767)
					buffer[i] = 32767;
				if (buffer[i] < -32768)
					buffer[i] = -32768;
			}
		}
		if (clipMode) {
			for (i = 0; i < samples; i++)
				buffer[i] = tanh(chan_buffers[i % 2][i / 2]) * finalGain;
		}
	}
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
