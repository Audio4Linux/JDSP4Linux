#ifdef DEBUG
#define TAG "EffectDSPMain"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#endif
#include "EffectDSPMain.h"
#include "firgen.h"

typedef struct
{
	int32_t status;
	uint32_t psize;
	uint32_t vsize;
	int32_t cmd;
	int32_t data;
} reply1x4_1x4_t;

EffectDSPMain::EffectDSPMain()
	: pregain(12.0f), threshold(-60.0f), knee(30.0f), ratio(12.0f), attack(0.001f), release(0.24f), inputBuffer(0), mPreset(0), mReverbMode(1), roomSize(50.0f), reverbEnabled(0)
	, fxreTime(0.5f), damping(0.5f), inBandwidth(0.8f), earlyLv(0.5f), tailLv(0.5f), verbL(0), mMatrixMCoeff(1.0), mMatrixSCoeff(1.0), bassBoostLp(0), convolver(0), tapsLPFIR(1024), clipMode(0), tempImpulseInt32(0), tempImpulseFloat(0), finalImpulse(0), convolverReady(-1), bassLpReady(-1)
{
}
EffectDSPMain::~EffectDSPMain()
{
	if (inputBuffer)
	{
		free(inputBuffer[0]);
		free(inputBuffer[1]);
		free(inputBuffer);
		free(outputBuffer[0]);
		free(outputBuffer[1]);
		free(outputBuffer);
	}
	if (verbL)
	{
		gverb_free(verbL);
		gverb_free(verbR);
	}
	if (bassBoostLp)
	{
		hcCloseSingle(&bassBoostLp[0]);
		hcCloseSingle(&bassBoostLp[1]);
		free(bassBoostLp);
	}
	if (convolver)
	{
		hcCloseSingle(&convolver[0]);
		hcCloseSingle(&convolver[1]);
		free(convolver);
	}
	if (finalImpulse)
	{
		free(finalImpulse[0]);
		free(finalImpulse[1]);
		free(finalImpulse);
	}
	if (tempImpulseInt32)
		free(tempImpulseInt32);
	if (tempImpulseFloat)
		free(tempImpulseFloat);
#ifdef DEBUG
	LOGI("Free buffer");
#endif
}
int32_t EffectDSPMain::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
	if (cmdCode == EFFECT_CMD_SET_CONFIG)
	{
		size_t frameCountInit;
		effect_buffer_access_e mAccessMode;
		int32_t *replyData = (int32_t *)pReplyData;
		int32_t ret = Effect::configure(pCmdData, &frameCountInit, &mAccessMode);
		if (ret != 0)
		{
			*replyData = ret;
			return 0;
		}
		memSize = frameCountInit * sizeof(float);
		if (!inputBuffer)
		{
			oldframeCountInit = frameCountInit;
			inputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			inputBuffer[0] = (float*)malloc(memSize);
			inputBuffer[1] = (float*)malloc(memSize);
			outputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			outputBuffer[0] = (float*)malloc(memSize);
			outputBuffer[1] = (float*)malloc(memSize);
#ifdef DEBUG
			LOGI("%d space allocated", frameCountInit);
#endif
		}
		if (frameCountInit != oldframeCountInit)
		{
			if (inputBuffer)
			{
				free(inputBuffer[0]);
				free(inputBuffer[1]);
				free(inputBuffer);
				free(outputBuffer[0]);
				free(outputBuffer[1]);
				free(outputBuffer);
			}
			inputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			inputBuffer[0] = (float*)malloc(memSize);
			inputBuffer[1] = (float*)malloc(memSize);
			outputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			outputBuffer[0] = (float*)malloc(memSize);
			outputBuffer[1] = (float*)malloc(memSize);
			oldframeCountInit = frameCountInit;
#ifdef DEBUG
			LOGI("%d space reallocated", frameCountInit);
#endif
		}
		*replyData = 0;
		return 0;
	}
	if (cmdCode == EFFECT_CMD_GET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		int32_t *replyData = (int32_t *)pReplyData;
		if (cep->psize == 4 && cep->vsize == 4)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 20000)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20000;
				replyData->data = (int32_t)mSamplingRate;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20001)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20001;
				if (!convolver)
					replyData->data = (int32_t)1;
				else
					replyData->data = (int32_t)0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
		}
	}
	if (cmdCode == EFFECT_CMD_SET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		int32_t *replyData = (int32_t *)pReplyData;
		if (cep->psize == 4 && cep->vsize == 2)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 100)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = pregain;
				pregain = (float)value;
				if (oldVal != pregain && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 101)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = threshold;
				threshold = (float)-value;
				if (oldVal != threshold && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 102)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = knee;
				knee = (float)value;
				if (oldVal != knee && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 103)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = ratio;
				ratio = (float)value;
				if (oldVal != ratio && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 104)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = attack;
				attack = value / 1000.0f;
				if (oldVal != attack && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 105)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = release;
				release = value / 1000.0f;
				if (oldVal != release && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 112)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostStrength;
				bassBoostStrength = value;
				if (oldVal != bassBoostStrength)
				{
					refreshBass();
					bassLpReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 113)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldbassBoostFilterType = bassBoostFilterType;
				bassBoostFilterType = value;
				if (oldbassBoostFilterType != bassBoostFilterType)
					refreshBass();
				if ((oldbassBoostFilterType != bassBoostFilterType) && bassBoostFilterType == 1)
				{
					tapsLPFIR = 1024;
					bassLpReady = -1;
				}
				else if ((oldbassBoostFilterType != bassBoostFilterType) && bassBoostFilterType == 2)
				{
					tapsLPFIR = 4096;
					bassLpReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 114)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldbassBoostCentreFreq = bassBoostCentreFreq;
				if (bassBoostCentreFreq < 55.0f)
					bassBoostCentreFreq = 55.0f;
				bassBoostCentreFreq = (float)value;
				if (oldbassBoostCentreFreq != bassBoostCentreFreq)
				{
					refreshBass();
					bassLpReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 127)
			{
				int16_t oldVal = mReverbMode;
				mReverbMode = ((int16_t *)cep)[8];
				if (oldVal != mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 128)
			{
				int16_t oldVal = mPreset;
				mPreset = ((int16_t *)cep)[8];
				if (oldVal != mPreset && !mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 129)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = roomSize;
				roomSize = (float)value;
				if (oldVal != roomSize && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 130)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = fxreTime;
				fxreTime = value / 100.0f;
				if (oldVal != fxreTime && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 131)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = damping;
				damping = value / 100.0f;
				if (oldVal != damping && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 133)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = inBandwidth;
				inBandwidth = value / 100.0f;
				if (oldVal != inBandwidth && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 134)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = earlyLv;
				earlyLv = value / 100.0f;
				if (oldVal != earlyLv && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 135)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = tailLv;
				tailLv = value / 100.0f;
				if (oldVal != tailLv && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 137)
			{
				int16_t value = ((int16_t *)cep)[8];
				refreshStereoWiden(value);
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1200)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = compressionEnabled;
				compressionEnabled = value;
				if (oldVal != compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1201)
			{
				bassBoostEnabled = ((int16_t *)cep)[8];
				if (!bassBoostEnabled)
				{
					if (bassBoostLp)
					{
						for (unsigned int i = 0; i < NUMCHANNEL; i++)
							hcCloseSingle(&bassBoostLp[i]);
						free(bassBoostLp);
						bassBoostLp = 0;
					}
					bassLpReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1202)
			{
				equalizerEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1203)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = reverbEnabled;
				reverbEnabled = value;
				if (oldVal != reverbEnabled)
					refreshReverb();
				if (verbL && !reverbEnabled)
				{
					gverb_flush(verbL);
					gverb_flush(verbR);
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1204)
			{
				stereoWidenEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1205)
			{
				convolverEnabled = ((int16_t *)cep)[8];
				if (!convolverEnabled)
				{
					if (convolver)
					{
						for (uint32_t i = 0; i < NUMCHANNEL; i++)
							hcCloseSingle(&convolver[i]);
						free(convolver);
						convolver = 0;
					}
					convolverReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 10003)
			{
				samplesInc = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 10004)
			{
				size_t i, j, tempbufValue;
				if (finalImpulse)
				{
					for (i = 0; i < impChannels; i++)
						free(finalImpulse[i]);
					free(finalImpulse);
					finalImpulse = 0;
				}
				if (!finalImpulse)
				{
					tempbufValue = impulseLengthActual * impChannels;
					tempImpulseFloat = (float*)malloc(tempbufValue * sizeof(float));
					for (i = 0; i < tempbufValue; i++)
						tempImpulseFloat[i] = ((float)((double)((int32_t)(tempImpulseInt32[i])) * 0.0000000004656612873077392578125));
					free(tempImpulseInt32);
					tempImpulseInt32 = 0;
					normalize(tempImpulseFloat, tempbufValue, 0.065f);
					finalImpulse = (float**)malloc(impChannels * sizeof(float*));
					for (i = 0; i < impChannels; i++)
					{
						float* channelbuf = (float*)malloc(impulseLengthActual * sizeof(float));
						float* p = tempImpulseFloat + i;
						for (j = 0; j < impulseLengthActual; j++)
							channelbuf[j] = p[j * impChannels];
						finalImpulse[i] = channelbuf;
					}
					convolverReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1500)
			{
				int16_t value = ((int16_t *)cep)[8];
				clipMode = value;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1501)
			{
				int16_t value = ((int16_t *)cep)[8];
				finalGain = value / 100.0f * 32768.0f;
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 115)
			{
				for (uint32_t i = 0; i < 10; i++)
					mBand[i] = ((int32_t *)cep)[4 + i] * 0.0001;
				refreshEqBands();
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 12)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 9999)
			{
				impulseLengthActual = ((int32_t *)cep)[4];
				impChannels = ((int32_t *)cep)[5];
				numTime2Send = ((int32_t *)cep)[6];
				if (tempImpulseInt32)
				{
					free(tempImpulseInt32);
					tempImpulseInt32 = 0;
				}
				if (!tempImpulseInt32)
					tempImpulseInt32 = (int32_t*)calloc(4096 * impChannels * numTime2Send, sizeof(int32_t));
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16384)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12000)
			{
				memcpy(tempImpulseInt32 + (samplesInc * 4096), ((int32_t *)cep) + 4, 4096 * sizeof(int32_t));
				*replyData = 0;
				return 0;
			}
		}
		return -1;
	}
	return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}
void EffectDSPMain::normalize(float* buffer, size_t num_samps, float maxval)
{
	size_t i;
	float loudest_sample = 0.0;
	float multiplier = 0.0;
	for (i = 0; i < num_samps; i++)
	{
		if (fabsf(buffer[i]) > loudest_sample) loudest_sample = buffer[i];
	}
	multiplier = maxval / loudest_sample;
	for (i = 0; i < num_samps; i++)
		buffer[i] *= multiplier;
}
void EffectDSPMain::refreshBassLinearPhase(uint32_t actualframeCount)
{
	float imp[4096];
	float winBeta = bassBoostCentreFreq / 30.0f;
	float strength = bassBoostStrength / 350.0f;
	if (strength < 1.0f)
		strength = 1.0f;
	if (tapsLPFIR < 1025)
		JfirLP(imp, tapsLPFIR, 0, bassBoostCentreFreq / mSamplingRate, winBeta, strength);
	else
		JfirLP(imp, tapsLPFIR, 1, bassBoostCentreFreq / mSamplingRate, winBeta, strength);
	unsigned int i;
	if (bassBoostLp)
	{
		for (i = 0; i < NUMCHANNEL; i++)
			hcCloseSingle(&bassBoostLp[i]);
		free(bassBoostLp);
		bassBoostLp = 0;
	}
	if (!bassBoostLp)
	{
		bassBoostLp = (HConvSingle*)malloc(sizeof(HConvSingle) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			hcInitSingle(&bassBoostLp[i], imp, tapsLPFIR, actualframeCount, 1, 0);
	}
#ifdef DEBUG
	LOGI("Linear phase FIR allocate all done: total taps %d", tapsLPFIR);
#endif
	bassLpReady = 1;
}
void EffectDSPMain::refreshConvolver(uint32_t actualframeCount)
{
	if (!finalImpulse)
		return;
#ifdef DEBUG
	LOGI("refreshConvolver::IR channel count:%d, IR frame count:%d, Audio buffer size:%d", impChannels, impulseLengthActual, actualframeCount);
#endif
	unsigned int i;
	if (convolver)
	{
		for (i = 0; i < NUMCHANNEL; i++)
			hcCloseSingle(&convolver[i]);
		free(convolver);
		convolver = 0;
	}
	if (!convolver)
	{
		convolver = (HConvSingle*)malloc(sizeof(HConvSingle) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			hcInitSingle(&convolver[i], finalImpulse[i % impChannels], impulseLengthActual, actualframeCount, 1, 0);
		if (finalImpulse)
		{
			for (i = 0; i < impChannels; i++)
				free(finalImpulse[i]);
			free(finalImpulse);
			finalImpulse = 0;
			free(tempImpulseFloat);
			tempImpulseFloat = 0;
		}
		convolverReady = 1;
#ifdef DEBUG
		LOGI("Convolver IR allocate complete");
#endif
	}
}
void EffectDSPMain::refreshStereoWiden(uint32_t parameter)
{
	switch (parameter)
	{
	case 0: // A Bit
		mMatrixMCoeff = 1.0f * 0.5f;
		mMatrixSCoeff = 1.2f * 0.5f;
		break;
	case 1: // Slight
		mMatrixMCoeff = 0.95f * 0.5f;
		mMatrixSCoeff = 1.4f * 0.5f;
		break;
	case 2: // Moderate
		mMatrixMCoeff = 0.9f * 0.5f;
		mMatrixSCoeff = 1.6f * 0.5f;
		break;
	case 3: // High
		mMatrixMCoeff = 0.85f * 0.5f;
		mMatrixSCoeff = 1.8f * 0.5f;
		break;
	case 4: // Super
		mMatrixMCoeff = 0.8f * 0.5f;
		mMatrixSCoeff = 2.0f * 0.5f;
		break;
	}
}
void EffectDSPMain::refreshCompressor()
{
	sf_advancecomp(&compressor, mSamplingRate, pregain, threshold, knee, ratio, attack, release, 0.003f, 0.09f, 0.16f, 0.42f, 0.98f, -(pregain / 1.4), 1.0f);
}
void EffectDSPMain::refreshEqBands()
{
	lsl.setup(4, mSamplingRate, 31.0, mBand[0]);
	lsr.setup(4, mSamplingRate, 31.0, mBand[0]);
	bs1l.setup(4, mSamplingRate, 61.0, 55.0, mBand[1]*0.92);
	bs1r.setup(4, mSamplingRate, 61.0, 55.0, mBand[1]*0.92);
	bs2l.setup(3, mSamplingRate, 124.0, 66.0, mBand[2]*0.86);
	bs2r.setup(3, mSamplingRate, 124.0, 66.0, mBand[2]*0.86);
	bs3l.setup(2, mSamplingRate, 250.0, 180.0, mBand[3]*0.92);
	bs3r.setup(2, mSamplingRate, 250.0, 180.0, mBand[3]*0.92);
	bs4l.setup(2, mSamplingRate, 500.0, 290.0, mBand[4]*0.91);
	bs4r.setup(2, mSamplingRate, 500.0, 290.0, mBand[4]*0.91);
	bs5l.setup(3, mSamplingRate, 1000.0, 720.0, mBand[5]*0.9);
	bs5r.setup(3, mSamplingRate, 1000.0, 720.0, mBand[5]*0.9);
	bs6l.setup(3, mSamplingRate, 2000.0, 1300.0, mBand[6]*0.88);
	bs6r.setup(3, mSamplingRate, 2000.0, 1300.0, mBand[6]*0.88);
	bs7l.setup(2, mSamplingRate, 4000.0, 2600.0, mBand[7]*0.93);
	bs7r.setup(2, mSamplingRate, 4000.0, 2600.0, mBand[7]*0.93);
	bs8l.setup(2, mSamplingRate, 10000.0, 8000.0, mBand[8]*0.99);
	bs8r.setup(2, mSamplingRate, 10000.0, 8000.0, mBand[8]*0.99);
	bs9l.setup(4, mSamplingRate, 15000.0, mBand[9]);
	bs9r.setup(4, mSamplingRate, 15000.0, mBand[9]);
}
void EffectDSPMain::refreshReverb()
{
	if (mReverbMode == 1)
	{
		if (verbL == NULL)
		{
			verbL = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, 50.0f, inBandwidth, earlyLv, tailLv);
			verbR = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, 50.0f, inBandwidth, earlyLv, tailLv);
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		else
		{
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
	else
	{
		//Refresh reverb memory when mode changed
		if (verbL != NULL)
		{
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		if (mPreset < 0 || mPreset > 18)
			mPreset = 0;
		sf_presetreverb(&myreverb, mSamplingRate, (sf_reverb_preset)mPreset);
	}
}
void EffectDSPMain::refreshBass()
{
	float gain = bassBoostStrength / 100.0f;
	bbL.setup(4, mSamplingRate, bassBoostCentreFreq, gain);
	bbR.setup(4, mSamplingRate, bassBoostCentreFreq, gain);
}
int32_t EffectDSPMain::process(audio_buffer_t *in, audio_buffer_t *out)
{
	size_t i, j;
	float outLL, outLR, outRL, outRR;
	channel_split(in->s16, in->frameCount, inputBuffer);
	if (bassBoostEnabled)
	{
		if (!bassBoostFilterType)
		{
			for (i = 0; i < in->frameCount; i++)
			{
				inputBuffer[0][i] = bbL.filter(inputBuffer[0][i]);
				inputBuffer[1][i] = bbR.filter(inputBuffer[1][i]);
			}
		}
		else if (bassBoostFilterType > 0)
		{
			if (bassLpReady > 0)
			{
				hcProcessAdd(&bassBoostLp[0], inputBuffer[0], inputBuffer[0]);
				hcProcessAdd(&bassBoostLp[1], inputBuffer[1], inputBuffer[1]);
			}
			else
				refreshBassLinearPhase(in->frameCount);
		}
	}
	if (equalizerEnabled)
	{
		for (i = 0; i < in->frameCount; i++)
		{
			outLL = lsl.filter(inputBuffer[0][i]);
			outRR = lsr.filter(inputBuffer[1][i]);
			outLL = bs1l.filter(outLL);
			outRR = bs1r.filter(outRR);
			outLL = bs2l.filter(outLL);
			outRR = bs2r.filter(outRR);
			outLL = bs3l.filter(outLL);
			outRR = bs3r.filter(outRR);
			outLL = bs4l.filter(outLL);
			outRR = bs4r.filter(outRR);
			outLL = bs5l.filter(outLL);
			outRR = bs5r.filter(outRR);
			outLL = bs6l.filter(outLL);
			outRR = bs6r.filter(outRR);
			outLL = bs7l.filter(outLL);
			outRR = bs7r.filter(outRR);
			outLL = bs8l.filter(outLL);
			outRR = bs8r.filter(outRR);
			inputBuffer[0][i] = bs9l.filter(outLL);
			inputBuffer[1][i] = bs9r.filter(outRR);
		}
	}
	if (reverbEnabled)
	{
		if (mReverbMode == 1)
		{
			for (i = 0; i < in->frameCount; i++)
			{
				gverb_do(verbL, inputBuffer[0][i], &outLL, &outLR);
				gverb_do(verbR, inputBuffer[1][i], &outRL, &outRR);
				inputBuffer[0][i] = outLL + outRL;
				inputBuffer[1][i] = outLR + outRR;
			}
		}
		else
		{
			for (i = 0; i < in->frameCount; i++)
				sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
		}
	}
	if (convolverEnabled)
	{
		if (convolverReady > 0)
		{
			size_t memsize = memSize;
			hcProcess(&convolver[0], inputBuffer[0], outputBuffer[0]);
			hcProcess(&convolver[1], inputBuffer[1], outputBuffer[1]);
			memcpy(inputBuffer[0], outputBuffer[0], memsize);
			memcpy(inputBuffer[1], outputBuffer[1], memsize);
		}
		else
			refreshConvolver(in->frameCount);
	}
	if (stereoWidenEnabled)
	{
		for (i = 0; i < in->frameCount; i++)
		{
			outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
			outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
			inputBuffer[0][i] = outLR + outRL;
			inputBuffer[1][i] = outLR - outRL;
		}
	}
	if (compressionEnabled)
	{
		sf_compressor_process(&compressor, in->frameCount, inputBuffer[0], inputBuffer[1], outputBuffer[0], outputBuffer[1]);
		channel_join(outputBuffer, out->s16, in->frameCount);
	}
	else
		channel_join(inputBuffer, out->s16, in->frameCount);
	return mEnable ? 0 : -ENODATA;
}
