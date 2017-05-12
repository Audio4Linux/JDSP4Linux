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
	// Float buffer **
	float **inputBuffer, **outputBuffer;
	sf_compressor_state_st compressor;
	ty_gverb *verbL, *verbR;
	sf_reverb_state_st myreverb;
	HConvSingle* bassBosstLp;
	float pregain, threshold, knee, ratio, attack, release, predelay, releasezone1, releasezone2, releasezone3, releasezone4, postgain;
	float bassBoostCentreFreq, finalGain, roomSize, fxreTime, damping, spread, deltaSpread, inBandwidth, earlyLv, tailLv, mMatrixMCoeff, mMatrixSCoeff;
	int16_t bassBoostStrength, bassBoostFilterType;
	float mBand[NUM_BANDS];
	int16_t compressionEnabled, bassBoostEnabled, equalizerEnabled, reverbEnabled, stereoWidenEnabled, normaliseEnabled, clipMode, convolverEnabled, convolverReady, bassParameterChanged;
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
	int16_t mPreset, mReverbMode, widenStrength;
	int tapsLPFIR;
	float softClip(float x, float k);
	void refreshCompressor();
	void refreshBass();
	void refreshBassLinearPhase(uint32_t frameCount);
	void refreshEqBands();
	void refreshReverb();
	void refreshStereoWiden();
	//    void refreshConvolver(uint32_t audioFrame);
	void normalise(float** buffer, int num_frames);
	inline void channel_split(int16_t* buffer, int num_frames, float** chan_buffers);
	inline void channel_join(float** chan_buffers, int16_t* buffer, int num_frames);
public:
	EffectDSPMain();
	~EffectDSPMain();
	int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
	int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
