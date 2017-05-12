#ifdef DEBUG
#define TAG "EffectDSPMain"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#endif
#include "EffectDSPMain.h"
#include "firgen.h"

EffectDSPMain::EffectDSPMain()
	: pregain(0), threshold(-20.0f), knee(30.0f), ratio(12.0f), attack(0.003f), release(0.25f), predelay(0.05f), releasezone1(0.1f), releasezone2(0.15f), releasezone3(0.4f), releasezone4(0.95f), inputBuffer(0), mPreset(0), mReverbMode(1), roomSize(50.0f)
	, fxreTime(0.5f), damping(0.5f), spread(50.0f), inBandwidth(0.8f), earlyLv(0.5f), tailLv(0.5f), verbL(0), mMatrixMCoeff(1.0), mMatrixSCoeff(1.0), bassBosstLp(0), clipMode(0)
{
}
EffectDSPMain::~EffectDSPMain()
{
	if (inputBuffer)
	{
		for (uint32_t i = 0; i < NUMCHANNEL; i++)
			free(inputBuffer[i]);
		free(inputBuffer);
		for (uint32_t i = 0; i < NUMCHANNEL; i++)
			free(outputBuffer[i]);
		free(outputBuffer);
#ifdef DEBUG
		LOGI("Free buffer");
#endif
	}
	if (verbL) {
		gverb_free(verbL);
		gverb_free(verbR);
	}
	if (bassBosstLp != NULL)
	{
		for (unsigned i = 0; i < NUMCHANNEL; i++)
			hcCloseSingle(&bassBosstLp[i]);
		free(bassBosstLp);
	}
}
int32_t EffectDSPMain::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
	if (cmdCode == EFFECT_CMD_SET_CONFIG) {
		int32_t *replyData = (int32_t *)pReplyData;
		int32_t ret = Effect::configure(pCmdData);
		if (ret != 0) {
			*replyData = ret;
			return 0;
		}
		if (inputBuffer == NULL)
		{
			inputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			for (uint32_t i = 0; i < NUMCHANNEL; i++)
				inputBuffer[i] = (float*)malloc(frameCountInit * sizeof(float));
			outputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			for (uint32_t i = 0; i < NUMCHANNEL; i++)
				outputBuffer[i] = (float*)malloc(frameCountInit * sizeof(float));
#ifdef DEBUG
			LOGI("%d space allocated", frameCountInit);
#endif
		}
		*replyData = 0;
		return 0;
	}

	if (cmdCode == EFFECT_CMD_SET_PARAM) {
		effect_param_t *cep = (effect_param_t *)pCmdData;
		int32_t *replyData = (int32_t *)pReplyData;
		/*		if (cep->psize == 4 && cep->vsize == 16384) {
		int32_t cmd = ((int32_t *)cep)[3];
		if (cmd == 13000) {
		short impReceived[8192];
		LOGI("cep->vsize:%d",cep->vsize);
		//for(unsigned int i = 0; i < cep->vsize; i++)
		//	LOGI("Element %d: %d",i, ((int16_t *)cep)[i]);
		memcpy(impReceived, ((int16_t *)cep)+8, 8192 * sizeof(int16_t));
		for(unsigned int i = 0; i < 8192; i++)
		LOGI("Element %d: %d",i, impReceived[i]);
		LOGI("Array CMD received");
		}
		}*/
		if (cep->psize == 4 && cep->vsize == 2) {
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 100) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = pregain;
				pregain = (float)value;
				if (oldBandVal != pregain && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 101) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = threshold;
				threshold = (float)-value;
				if (oldBandVal != threshold && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 102) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = knee;
				knee = (float)value;
				if (oldBandVal != knee && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 103) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = ratio;
				ratio = (float)value;
				if (oldBandVal != ratio && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 104) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = attack;
				attack = value / 1000.0f;
				if (oldBandVal != attack && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 105) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = release;
				release = value / 1000.0f;
				if (oldBandVal != release && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 106) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = predelay;
				predelay = value / 10000.0f;
				if (oldBandVal != predelay && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 107) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = releasezone1;
				releasezone1 = value / 1000.0f;
				if (oldBandVal != releasezone1 && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 108) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = releasezone2;
				releasezone2 = value / 1000.0f;
				if (oldBandVal != releasezone2 && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 109) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = releasezone3;
				releasezone3 = value / 1000.0f;
				if (oldBandVal != releasezone3 && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 110) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = releasezone4;
				releasezone4 = value / 1000.0f;
				if (oldBandVal != releasezone4 && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 111) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = postgain;
				postgain = (float)value;
				if (oldBandVal != postgain && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 112) {
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldBandVal = bassBoostStrength;
				bassBoostStrength = value;
				if (oldBandVal != bassBoostStrength) {
					refreshBass();
					bassParameterChanged = 1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 113) {
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldBandVal = bassBoostFilterType;
				bassBoostFilterType = value;
				if (oldBandVal != bassBoostFilterType)
					refreshBass();
				if ((oldBandVal != bassBoostFilterType) && bassBoostFilterType == 1) {
					tapsLPFIR = 1024;
					bassParameterChanged = 1;
				}
				else if ((oldBandVal != bassBoostFilterType) && bassBoostFilterType == 2) {
					tapsLPFIR = 4096;
					bassParameterChanged = 1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 114) {
				int16_t value = ((int16_t *)cep)[8];
				float oldBandVal = bassBoostCentreFreq;
				if (bassBoostCentreFreq < 55.0f)
					bassBoostCentreFreq = 55.0f;
				bassBoostCentreFreq = (float)value;
				if (oldBandVal != bassBoostCentreFreq) {
					refreshBass();
					bassParameterChanged = 1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 115) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[0] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 116) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[1] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 117) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[2] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 118) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[3] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 119) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[4] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 120) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[5] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 121) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[6] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 122) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[7] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 123) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[8] = value / 100.0f;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 124) {
				int16_t value = ((int16_t *)cep)[8];
				mBand[9] = value / 100.0f;
				refreshEqBands();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 127) {
				int16_t oldVal = mReverbMode;
				mReverbMode = ((int16_t *)cep)[8];
				if (oldVal != mReverbMode && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 128) {
				int16_t oldVal = mPreset;
				mPreset = ((int16_t *)cep)[8];
				if (oldVal != mPreset && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 129) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = roomSize;
				roomSize = (float)value;
				if (oldVal != roomSize && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 130) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = fxreTime;
				fxreTime = value / 100.0f;
				if (oldVal != fxreTime && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 131) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = damping;
				damping = value / 100.0f;
				if (oldVal != damping && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 132) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = spread;
				spread = (float)value;
				if (oldVal != spread && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 133) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = inBandwidth;
				inBandwidth = value / 100.0f;
				if (oldVal != inBandwidth && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 134) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = earlyLv;
				earlyLv = value / 100.0f;
				if (oldVal != earlyLv && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 135) {
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = tailLv;
				tailLv = value / 100.0f;
				if (oldVal != tailLv && reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 137) {
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = widenStrength;
				widenStrength = value;
				if (oldVal != widenStrength && reverbEnabled)
					refreshStereoWiden();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1200) {
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = compressionEnabled;
				compressionEnabled = value;
				if (oldVal != compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1201) {
				bassBoostEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1202) {
				equalizerEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1203) {
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = reverbEnabled;
				reverbEnabled = value;
				if (verbL != NULL) {
					gverb_flush(verbL);
					gverb_flush(verbR);
				}
				if (oldVal != reverbEnabled)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1204) {
				stereoWidenEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1205) {
				convolverEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1500) {
				int16_t value = ((int16_t *)cep)[8];
				clipMode = value;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1501) {
				normaliseEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1502) {
				int16_t value = ((int16_t *)cep)[8];
				finalGain = value / 100.0f * 32767.0f;
				*replyData = 0;
				return 0;
			}
		}
		return -1;
	}
	return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}
void EffectDSPMain::refreshReverb()
{
	if (mReverbMode == 1) {
		if (verbL == NULL) {
			verbL = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, spread, inBandwidth, earlyLv, tailLv);
			verbR = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, spread, inBandwidth, earlyLv, tailLv);
			deltaSpread = spread;
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		else if (deltaSpread != spread)
		{
			gverb_free(verbL);
			gverb_free(verbR);
			verbL = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, spread, inBandwidth, earlyLv, tailLv);
			verbR = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, spread, inBandwidth, earlyLv, tailLv);
			deltaSpread = spread;
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		else {
			//Add more reverb control parameters here!
			gverb_set_roomsize(verbL, roomSize);
			gverb_set_roomsize(verbR, roomSize);
			gverb_set_revtime(verbL, fxreTime);
			gverb_set_revtime(verbR, fxreTime);
			gverb_set_damping(verbL, damping);
			gverb_set_damping(verbR, damping);
			gverb_set_inputbandwidth(verbL, inBandwidth);
			gverb_set_inputbandwidth(verbR, inBandwidth);
			gverb_set_earlylevel(verbL, earlyLv);
			gverb_set_earlylevel(verbR, earlyLv);
			gverb_set_taillevel(verbL, tailLv);
			gverb_set_taillevel(verbR, tailLv);
		}
	}
	else {
		//Refresh reverb memory when mode changed
		if (verbL != NULL) {
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		if (mPreset < 0 || mPreset > 18)
			mPreset = 0;
		sf_presetreverb(&myreverb, mSamplingRate, (sf_reverb_preset)mPreset);
	}
}
void EffectDSPMain::refreshStereoWiden()
{
	switch (widenStrength) {
	case 0: // A Bit
		mMatrixMCoeff = 1.0;
		mMatrixSCoeff = 1.2;
		break;
	case 1: // Slight
		mMatrixMCoeff = 0.95;
		mMatrixSCoeff = 1.4;
		break;
	case 2: // Moderate
		mMatrixMCoeff = 0.90;
		mMatrixSCoeff = 1.7;
		break;
	case 3: // High
		mMatrixMCoeff = 0.80;
		mMatrixSCoeff = 2.0;
		break;
	case 4: // Super
		mMatrixMCoeff = 0.70;
		mMatrixSCoeff = 2.5;
		break;
	}
}
void EffectDSPMain::refreshBass()
{
	float gain = bassBoostStrength / 100.0;
	bbL.setup(4, mSamplingRate, bassBoostCentreFreq, gain);
	bbR.setup(4, mSamplingRate, bassBoostCentreFreq, gain);
}
void EffectDSPMain::refreshBassLinearPhase(uint32_t actualframeCount)
{
	float imp[4096];
	float winBeta = bassBoostCentreFreq / 10.0f;
	float strength = bassBoostStrength / 350.0f;
	if (strength < 1.0f)
		strength = 1.0f;
	JfirLP(imp, tapsLPFIR, 1, bassBoostCentreFreq / mSamplingRate, winBeta, strength);
	if (bassBosstLp != NULL)
	{
		for (unsigned i = 0; i < NUMCHANNEL; i++)
			hcCloseSingle(&bassBosstLp[i]);
		free(bassBosstLp);
		bassBosstLp = NULL;
	}
	if (bassBosstLp == NULL) {
		bassBosstLp = (HConvSingle*)malloc(sizeof(HConvSingle) * NUMCHANNEL);
		for (unsigned i = 0; i < NUMCHANNEL; i++)
		{
			hcInitSingle(&bassBosstLp[i], imp, tapsLPFIR, actualframeCount, 1, 0);
		}
	}
#ifdef DEBUG
	LOGI("Linear phase FIR allocate all done: total taps %d", tapsLPFIR);
#endif
	bassParameterChanged = 0;
}
void EffectDSPMain::refreshEqBands()
{
	lsl.setup(4, mSamplingRate, 31.0, mBand[0]);
	lsr.setup(4, mSamplingRate, 31.0, mBand[0]);
	bs1l.setup(4, mSamplingRate, 62.0f, 52.0, mBand[1]);
	bs1r.setup(4, mSamplingRate, 62.0f, 52.0, mBand[1]);
	bs2l.setup(3, mSamplingRate, 125.0f, 66.0, mBand[2]);
	bs2r.setup(3, mSamplingRate, 125.0f, 66.0, mBand[2]);
	bs3l.setup(2, mSamplingRate, 250.0f, 180.0, mBand[3]);
	bs3r.setup(2, mSamplingRate, 250.0f, 180.0, mBand[3]);
	bs4l.setup(2, mSamplingRate, 500.0f, 300.0, mBand[4]);
	bs4r.setup(2, mSamplingRate, 500.0f, 300.0, mBand[4]);
	bs5l.setup(3, mSamplingRate, 1000.0f, 800.0, mBand[5]);
	bs5r.setup(3, mSamplingRate, 1000.0f, 800.0, mBand[5]);
	bs6l.setup(3, mSamplingRate, 2000.0f, 1200.0, mBand[6]);
	bs6r.setup(3, mSamplingRate, 2000.0f, 1200.0, mBand[6]);
	bs7l.setup(2, mSamplingRate, 4000.0f, 2800.0, mBand[7]);
	bs7r.setup(2, mSamplingRate, 4000.0f, 2800.0, mBand[7]);
	bs8l.setup(2, mSamplingRate, 8000.0f, 3600.0, mBand[8]);
	bs8r.setup(2, mSamplingRate, 8000.0f, 3600.0, mBand[8]);
	bs9l.setup(4, mSamplingRate, 16000.0f, mBand[9] * 1.1);
	bs9r.setup(4, mSamplingRate, 16000.0f, mBand[9] * 1.1);
}
void EffectDSPMain::refreshCompressor()
{
	sf_advancecomp(&compressor, mSamplingRate, pregain, threshold, knee, ratio, attack, release,
		predelay,     // seconds, length of the predelay buffer [0 to 1]
		releasezone1, // release zones should be increasing between 0 and 1, and are a fraction
		releasezone2, //  of the release time depending on the input dB -- these parameters define
		releasezone3, //  the adaptive release curve, which is discussed in further detail in the
		releasezone4, //  demo: adaptive-release-curve.html
		postgain,     // dB, amount of gain to apply after compression [0 to 100]
		1.0f);
}

int32_t EffectDSPMain::process(audio_buffer_t *in, audio_buffer_t *out)
{
	channel_split(in->s16, in->frameCount, inputBuffer);
	if (bassBoostEnabled) {
		if (bassBoostFilterType == 0) {
			for (uint32_t i = 0; i < in->frameCount; i++) {
				inputBuffer[0][i] = bbL.filter(inputBuffer[0][i]);
				inputBuffer[1][i] = bbR.filter(inputBuffer[1][i]);
			}
		}
		else if (bassBoostFilterType > 0)
		{
			if ((bassBosstLp == NULL) || bassParameterChanged)
				refreshBassLinearPhase(in->frameCount);
			for (unsigned i = 0; i < NUMCHANNEL; i++)
			{
				float* inputChannel = inputBuffer[i];
				float* outputChannel = outputBuffer[i];
				HConvSingle* bassBosstLpPtr = &bassBosstLp[i];
				hcPutSingle(bassBosstLpPtr, inputChannel);
				hcProcessSingle(bassBosstLpPtr);
				hcGetSingle(bassBosstLpPtr, outputChannel);
			}
			for (unsigned i = 0; i < NUMCHANNEL; i++)
			{
				for (unsigned j = 0; j < in->frameCount; j++)
					inputBuffer[i][j] += outputBuffer[i][j];
			}
		}
	}
	if (equalizerEnabled) {
		for (uint32_t i = 0; i < in->frameCount; i++) {
			float outL = lsl.filter(inputBuffer[0][i]);
			float outR = lsr.filter(inputBuffer[1][i]);
			outL = bs1l.filter(outL);
			outR = bs1r.filter(outR);
			outL = bs2l.filter(outL);
			outR = bs2r.filter(outR);
			outL = bs3l.filter(outL);
			outR = bs3r.filter(outR);
			outL = bs4l.filter(outL);
			outR = bs4r.filter(outR);
			outL = bs5l.filter(outL);
			outR = bs5r.filter(outR);
			outL = bs6l.filter(outL);
			outR = bs6r.filter(outR);
			outL = bs7l.filter(outL);
			outR = bs7r.filter(outR);
			outL = bs8l.filter(outL);
			outR = bs8r.filter(outR);
			inputBuffer[0][i] = bs9l.filter(outL);
			inputBuffer[1][i] = bs9r.filter(outR);
		}
	}
	if (reverbEnabled) {
		float outLL, outLR, outRL, outRR;
		if (mReverbMode == 1) {
			for (uint32_t i = 0; i < in->frameCount; i++) {
				gverb_do(verbL, inputBuffer[0][i], &outLL, &outLR);
				gverb_do(verbR, inputBuffer[1][i], &outRL, &outRR);
				inputBuffer[0][i] = (inputBuffer[0][i] * 0.2) + outLL + outRL;
				inputBuffer[1][i] = (inputBuffer[1][i] * 0.2) + outLR + outRR;
			}
		}
		else {
			for (uint32_t i = 0; i < in->frameCount; i++) {
				sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &outLL, &outRR);
				inputBuffer[0][i] = (inputBuffer[0][i] * 0.2) + outLL;
				inputBuffer[1][i] = (inputBuffer[1][i] * 0.2) + outRR;
			}
		}
	}
	if (stereoWidenEnabled) {
		for (uint32_t i = 0; i < in->frameCount; i++) {
			float M = (inputBuffer[0][i] + inputBuffer[1][i]) * 0.5f * mMatrixMCoeff;
			float S = (inputBuffer[0][i] - inputBuffer[1][i]) * 0.5f * mMatrixSCoeff;
			inputBuffer[0][i] = M + S;
			inputBuffer[1][i] = M - S;
		}
	}
	if (compressionEnabled) {
		sf_compressor_process(&compressor, in->frameCount, inputBuffer[0], inputBuffer[1], outputBuffer[0], outputBuffer[1]);
		channel_join(outputBuffer, out->s16, in->frameCount);
	}
	else
		channel_join(inputBuffer, out->s16, in->frameCount);
	return mEnable ? 0 : -ENODATA;
}

inline void EffectDSPMain::channel_split(int16_t* buffer, int num_frames, float** chan_buffers)
{
	int i;
	int samples = num_frames * 2;
	for (i = 0; i < samples; i++)
	{
		chan_buffers[(i % 2)][i / 2] = (float)((double)((int16_t)(buffer[i])) * 0.000030517578125);
	}
}
inline void EffectDSPMain::channel_join(float** chan_buffers, int16_t* buffer, int num_frames)
{
	int i;
	int samples = num_frames * 2;
	if (!clipMode) {
		if (normaliseEnabled)
			normalise(chan_buffers, samples);
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
		if (normaliseEnabled)
			normalise(chan_buffers, samples);
		for (i = 0; i < samples; i++)
		{
			buffer[i] = tanh(chan_buffers[i % 2][i / 2]) * finalGain;
		}
	}
}
void EffectDSPMain::normalise(float** buffer, int num_frames)
{
	float maxSamples = 0;
	float multiplier = 0;
	int i;
	for (i = 0; i < num_frames; i++)
		if (fabsf(buffer[i % 2][i / 2]) > maxSamples) maxSamples = buffer[i % 2][i / 2];
	if (maxSamples > 1.0f) {
		multiplier = 0.8f / maxSamples;
		for (i = 0; i < num_frames; i++)
			buffer[i % 2][i / 2] *= multiplier;
	}
}
