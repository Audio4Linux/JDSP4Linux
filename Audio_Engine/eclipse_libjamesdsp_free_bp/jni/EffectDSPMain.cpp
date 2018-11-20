#ifdef DEBUG
#define TAG "EffectDSPMain"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#include "MemoryUsage.h"
#endif
#include <unistd.h>
#include "EffectDSPMain.h"
typedef struct
{
	int32_t status;
	uint32_t psize;
	uint32_t vsize;
	int32_t cmd;
	int32_t data;
} reply1x4_1x4_t;
const double interpFreq[NUM_BANDS] = { 25.0, 40.0, 63.0, 100.0, 160.0, 250.0, 400.0, 630.0, 1000.0, 1600.0, 2500.0, 4000.0, 6300.0, 10000.0, 16000.0 };

EffectDSPMain::EffectDSPMain()
	: DSPbufferLength(1024), inOutRWPosition(0), equalizerEnabled(0), ramp(1.0), pregain(12.0), threshold(-60.0), knee(30.0), ratio(12.0), attack(0.001), release(0.24), isBenchData(0), mPreset(0), reverbEnabled(0)
	, mMatrixMCoeff(1.0), mMatrixSCoeff(1.0), bassBoostLp(0), FIREq(0), convolver(0), fullStereoConvolver(0), sosCount(0), resampledSOSCount(0), usedSOSCount(0), df441(0), df48(0), dfResampled(0)
	, tempImpulseIncoming(0), tempImpulsedouble(0), finalImpulse(0), convolverReady(-1), bassLpReady(-1), analogModelEnable(0), tubedrive(2.0), eqFilterType(0), arbEq(0), xaxis(0), yaxis(0), eqFIRReady(0)
{
	double c0[12] = { 2.138018534150542e-5, 4.0608501987194246e-5, 7.950414700590711e-5, 1.4049065318523225e-4, 2.988065284903209e-4, 0.0013061668170781858, 0.0036204239724680425, 0.008959629624060151, 0.027083658741258742, 0.08156916666666666, 0.1978822177777778, 0.4410733777777778 };
	double c1[12] = { 5.88199398839289e-6, 1.1786813951189911e-5, 2.5600214528512222e-5, 8.53041086120132e-5, 2.656291374239004e-4, 5.047717001008378e-4, 8.214255850540808e-4, 0.0016754651127819551, 0.0033478867132867136, 0.006705333333333334, 0.013496382222222221, 0.02673028888888889 };
	benchmarkValue[0] = (double*)malloc(12 * sizeof(double));
	benchmarkValue[1] = (double*)malloc(12 * sizeof(double));
	memcpy(benchmarkValue[0], c0, sizeof(c0));
	memcpy(benchmarkValue[1], c1, sizeof(c1));
	memSize = DSPbufferLength * sizeof(double);
	inputBuffer[0] = (double*)malloc(memSize);
	inputBuffer[1] = (double*)malloc(memSize);
	outputBuffer[0] = (double*)malloc(memSize);
	outputBuffer[1] = (double*)malloc(memSize);
	memset(outputBuffer[0], 0, memSize);
	memset(outputBuffer[1], 0, memSize);
	tempBuf[0] = (double*)malloc(memSize);
	tempBuf[1] = (double*)malloc(memSize);
#ifdef DEBUG
	LOGI("%d space allocated", DSPbufferLength);
#endif
	JLimiterInit(&kLimiter);
	JLimiterSetCoefficients(&kLimiter, -0.1, 60.0, mSamplingRate);
}
EffectDSPMain::~EffectDSPMain()
{
	if (inputBuffer[0])
	{
		free(inputBuffer[0]);
		inputBuffer[0] = 0;
		free(inputBuffer[1]);
		free(outputBuffer[0]);
		free(outputBuffer[1]);
		free(tempBuf[0]);
		free(tempBuf[1]);
	}
	FreeBassBoost();
	FreeEq();
	FreeConvolver();
	if (finalImpulse)
	{
		free(finalImpulse[0]);
		free(finalImpulse[1]);
		free(finalImpulse);
	}
	if (tempImpulseIncoming)
		free(tempImpulseIncoming);
	if (tempImpulsedouble)
		free(tempImpulsedouble);
	if (benchmarkValue[0])
		free(benchmarkValue[0]);
	if (benchmarkValue[1])
		free(benchmarkValue[1]);
	if (sosCount)
	{
		for (int i = 0; i < sosCount; i++)
		{
			free(df441[i]);
			free(df48[i]);
		}
		free(df441);
		free(df48);
		sosCount = 0;
		if (resampledSOSCount)
		{
			for (int i = 0; i < resampledSOSCount; i++)
				free(dfResampled[i]);
			free(dfResampled);
			dfResampled = 0;
			resampledSOSCount = 0;
		}
	}
#ifdef DEBUG
	LOGI("Buffer freed");
#endif
}
void EffectDSPMain::FreeBassBoost()
{
	if (bassBoostLp)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolver1x1Free(bassBoostLp[i]);
			free(bassBoostLp[i]);
		}
		free(bassBoostLp);
		bassBoostLp = 0;
		ramp = 0.4;
	}
}
void EffectDSPMain::FreeEq()
{
	if (xaxis)
	{
		free(xaxis);
		xaxis = 0;
	}
	if (yaxis)
	{
		free(yaxis);
		yaxis = 0;
	}
	if (arbEq)
	{
		ArbitraryEqFree(arbEq);
		free(arbEq);
		arbEq = 0;
	}
	if (FIREq)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolver1x1Free(FIREq[i]);
			free(FIREq[i]);
		}
		free(FIREq);
		FIREq = 0;
	}
}
void EffectDSPMain::FreeConvolver()
{
	convolverReady = -1;
	int i;
	if (convolver)
	{
		for (i = 0; i < 2; i++)
		{
			if (convolver[i])
			{
				AutoConvolver1x1Free(convolver[i]);
				free(convolver[i]);
				convolver[i] = 0;
			}
		}
		free(convolver);
		convolver = 0;
	}
	if (fullStereoConvolver)
	{
		for (i = 0; i < 4; i++)
		{
			if (fullStereoConvolver[i])
			{
				AutoConvolver1x1Free(fullStereoConvolver[i]);
				free(fullStereoConvolver[i]);
				fullStereoConvolver[i] = 0;
			}
		}
		free(fullStereoConvolver);
		fullStereoConvolver = 0;
	}
}
void EffectDSPMain::channel_splitFloat(const float *buffer, unsigned int num_frames, float **chan_buffers, unsigned int num_channels)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		chan_buffers[i % num_channels][i / num_channels] = buffer[i];
}
int32_t EffectDSPMain::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
#ifdef DEBUG
	LOGI("Memory used: %lf Mb", (double)getCurrentRSS() / 1024.0 / 1024.0);
#endif
	if (cmdCode == EFFECT_CMD_SET_CONFIG)
	{
		effect_buffer_access_e mAccessMode;
		int32_t *replyData = (int32_t *)pReplyData;
		int32_t ret = Effect::configure(pCmdData, &mAccessMode);
		if (ret != 0)
		{
			*replyData = ret;
			return 0;
		}
		JLimiterSetCoefficients(&kLimiter, -0.1, 60.0, mSamplingRate);
		fullStconvparams.in = inputBuffer;
		fullStconvparams.frameCount = DSPbufferLength;
		fullStconvparams1.in = inputBuffer;
		fullStconvparams1.frameCount = DSPbufferLength;
		rightparams2.in = inputBuffer;
		rightparams2.frameCount = DSPbufferLength;
		*replyData = 0;
		return 0;
	}
	if (cmdCode == EFFECT_CMD_GET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		if (cep->psize == 4 && cep->vsize == 4)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 19999)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 19999;
				replyData->data = (int32_t)DSPbufferLength;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20000)
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
			else if (cmd == 20002)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20002;
				replyData->data = (int32_t)getpid();
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20003)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (isBenchData == 2)
					replyData->data = isBenchData;
				else
					replyData->data = 0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20004)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (convolver)
					replyData->data = convolver[0]->methods;
				else if (fullStereoConvolver)
					replyData->data = fullStereoConvolver[0]->methods;
				else
					replyData->data = 0;
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
				double oldVal = pregain;
				pregain = (double)value;
				if (oldVal != pregain && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 101)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = threshold;
				threshold = (double)-value;
				if (oldVal != threshold && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 102)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = knee;
				knee = (double)value;
				if (oldVal != knee && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 103)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = ratio;
				ratio = (double)value;
				if (oldVal != ratio && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 104)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = attack;
				attack = value / 1000.0f;
				if (oldVal != attack && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 105)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = release;
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
					bassLpReady = 0;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 113)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostFilterType;
				bassBoostFilterType = value;
				if (oldVal != bassBoostFilterType)
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 114)
			{
				int16_t value = ((int16_t *)cep)[8];
				double oldVal = bassBoostCentreFreq;
				if (bassBoostCentreFreq < 55.0)
					bassBoostCentreFreq = 55.0;
				bassBoostCentreFreq = (double)value;
				if ((oldVal != bassBoostCentreFreq) || !bassLpReady)
				{
					bassLpReady = 0;
					if (!bassBoostFilterType)
						refreshBassLinearPhase(DSPbufferLength, 2048, bassBoostCentreFreq);
					else
						refreshBassLinearPhase(DSPbufferLength, 4096, bassBoostCentreFreq);
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 128)
			{
				int16_t oldVal = mPreset;
				mPreset = ((int16_t *)cep)[8];
				if (oldVal != mPreset)
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
			else if (cmd == 188)
			{
				int16_t value = ((int16_t *)cep)[8];
				if (bs2bEnabled == 2)
				{
					if (value == 0)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_JMEIER_CLEVEL);
					else if (value == 1)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_CMOY_CLEVEL);
					else if (value == 2)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_DEFAULT_CLEVEL);
					bs2bEnabled = 1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 150)
			{
				double oldVal = tubedrive;
				tubedrive = ((int16_t *)cep)[8] / 1000.0;
				if (analogModelEnable && oldVal != tubedrive)
					refreshTubeAmp();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 151)
			{
				int16_t oldVal = eqFilterType;
				int16_t value = ((int16_t *)cep)[8];
				if (oldVal != value)
				{
					eqFilterType = value;
					eqFIRReady = 2;
#ifdef DEBUG
					LOGI("EQ filter type: %d", eqFilterType);
#endif
					FreeEq();
#ifdef DEBUG
					LOGI("FIR EQ reseted caused by filter type change");
#endif
				}
				*replyData = 0;
				return 0;
			}
			/*			else if (cmd == 808)
			{
			double oldVal = tubedrive;
			tubedrive = ((int16_t *)cep)[8] / 1000.0f;
			if(wavechild670Enabled == -1)
			{
			Real inputLevelA = tubedrive;
			Real ACThresholdA = 0.35; // This require 0 < ACThresholdA < 1.0
			uint timeConstantSelectA = 1; // Integer from 1-6
			Real DCThresholdA = 0.35; // This require 0 < DCThresholdA < 1.0
			Real outputGain = 1.0 / inputLevelA;
			int sidechainLink = 0;
			int isMidSide = 0;
			int useFeedbackTopology = 1;
			Real inputLevelB = inputLevelA;
			Real ACThresholdB = ACThresholdA;
			uint timeConstantSelectB = timeConstantSelectA;
			Real DCThresholdB = DCThresholdA;
			Wavechild670Parameters params = Wavechild670ParametersInit(inputLevelA, ACThresholdA, timeConstantSelectA, DCThresholdA,
			inputLevelB, ACThresholdB, timeConstantSelectB, DCThresholdB,
			sidechainLink, isMidSide, useFeedbackTopology, outputGain);
			if (compressor670)
			free(compressor670);
			compressor670 = Wavechild670Init((Real)mSamplingRate, &params);
			Wavechild670WarmUp(compressor670, 0.5);
			wavechild670Enabled = 1;
			#ifdef DEBUG
			LOGI("Compressor670 Initialised");
			#endif
			}
			*replyData = 0;
			return 0;
			}*/
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
				int16_t oldVal = bassBoostEnabled;
				bassBoostEnabled = ((int16_t *)cep)[8];
				if (!bassBoostEnabled && (oldVal != bassBoostEnabled))
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1202)
			{
				int16_t oldVal = equalizerEnabled;
				equalizerEnabled = ((int16_t *)cep)[8];
				if ((equalizerEnabled == 1 && (oldVal != equalizerEnabled)) || eqFIRReady == 2)
				{
					xaxis = (double*)malloc(1024 * sizeof(double));
					yaxis = (double*)malloc(1024 * sizeof(double));
					linspace(xaxis, 1024, interpFreq[0], interpFreq[NUM_BANDSM1]);
					arbEq = (ArbitraryEq*)malloc(sizeof(ArbitraryEq));
					eqfilterLength = 8192;
					InitArbitraryEq(arbEq, &eqfilterLength, eqFilterType);
					for (int i = 0; i < 1024; i++)
						ArbitraryEqInsertNode(arbEq, xaxis[i], 0.0, 0);
#ifdef DEBUG
					LOGI("FIR EQ Initialised");
#endif
				}
				else if (!equalizerEnabled && (oldVal != equalizerEnabled))
				{
					eqFIRReady = 0;
					FreeEq();
#ifdef DEBUG
					LOGI("FIR EQ destroyed");
#endif
				}
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
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1204)
			{
				stereoWidenEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1208)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val)
					bs2bEnabled = 2;
				else
					bs2bEnabled = 0;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1205)
			{
				convolverEnabled = ((int16_t *)cep)[8];
				if (!convolverEnabled)
					FreeConvolver();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1206)
			{
				int16_t oldVal = analogModelEnable;
				analogModelEnable = ((int16_t *)cep)[8];
				if (analogModelEnable && oldVal != analogModelEnable)
					refreshTubeAmp();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1212)
			{
				int16_t oldVal = viperddcEnabled;
				if (mSamplingRate == 44100.0 && df441)
				{
					sosPointer = df441;
					usedSOSCount = sosCount;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
				else if (mSamplingRate == 48000.0 && df48)
				{
					sosPointer = df48;
					usedSOSCount = sosCount;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
				else
				{
					resampledSOSCount = PeakingFilterResampler(df48, 48000.0, &dfResampled, mSamplingRate, sosCount);
					usedSOSCount = resampledSOSCount;
					sosPointer = dfResampled;
					viperddcEnabled = ((int16_t *)cep)[8];
				}
#ifdef DEBUG
				LOGI("viperddcEnabled: %d", viperddcEnabled);
#endif
				if (!viperddcEnabled && oldVal)
				{
					if (stringEq)
					{
						free(stringEq);
						stringEq = 0;
					}
					if (sosCount)
					{
						for (int i = 0; i < sosCount; i++)
						{
							free(df441[i]);
							free(df48[i]);
						}
						free(df441);
						df441 = 0;
						free(df48);
						df48 = 0;
						sosCount = 0;
						if (resampledSOSCount)
						{
							for (int i = 0; i < resampledSOSCount; i++)
								free(dfResampled[i]);
							free(dfResampled);
							dfResampled = 0;
							resampledSOSCount = 0;
						}
						sosPointer = 0;
					}
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
				int i, j, tempbufValue;
				if (finalImpulse)
				{
					for (i = 0; i < previousimpChannels; i++)
						free(finalImpulse[i]);
					free(finalImpulse);
					finalImpulse = 0;
				}
				if (!finalImpulse)
				{
					tempbufValue = impulseLengthActual * impChannels;
					tempImpulsedouble = (double*)malloc(tempbufValue * sizeof(double));
					if (!tempImpulsedouble)
					{
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
					}
					for (i = 0; i < tempbufValue; i++)
						tempImpulsedouble[i] = (double)tempImpulseIncoming[i];
					free(tempImpulseIncoming);
					tempImpulseIncoming = 0;
					finalImpulse = (double**)malloc(impChannels * sizeof(double*));
					for (i = 0; i < impChannels; i++)
					{
						double* channelbuf = (double*)malloc(impulseLengthActual * sizeof(double));
						if (!channelbuf)
						{
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
							free(finalImpulse);
							finalImpulse = 0;
						}
						double* p = tempImpulsedouble + i;
						for (j = 0; j < impulseLengthActual; j++)
							channelbuf[j] = p[j * impChannels];
						finalImpulse[i] = channelbuf;
					}
					if (!refreshConvolver(DSPbufferLength))
					{
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
						if (finalImpulse)
						{
							for (i = 0; i < impChannels; i++)
								free(finalImpulse[i]);
							free(finalImpulse);
							finalImpulse = 0;
						}
						if (tempImpulsedouble)
						{
							free(tempImpulsedouble);
							tempImpulsedouble = 0;
						}
					}
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 10005)
			{
				stringIndex = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 10009)
			{
				if (!viperddcEnabled)
				{
					if (sosCount)
					{
						for (int i = 0; i < sosCount; i++)
						{
							free(df441[i]);
							free(df48[i]);
						}
						free(df441);
						df441 = 0;
						free(df48);
						df48 = 0;
						sosCount = 0;
						if (resampledSOSCount)
						{
							for (int i = 0; i < resampledSOSCount; i++)
								free(dfResampled[i]);
							free(dfResampled);
							dfResampled = 0;
							resampledSOSCount = 0;
						}
						sosPointer = 0;
					}
				}
#ifdef DEBUG
				LOGI("%s", stringEq);
#endif
				sosCount = DDCParser(stringEq, &df441, &df48);
#ifdef DEBUG
			LOGI("VDC num of SOS: %d", sosCount);
			LOGI("VDC df48[0].b0: %1.14lf", df48[0]->b0);
#endif
				free(stringEq);
				stringEq = 0;
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 8)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1500)
			{
				double limThreshold = (double)((float*)cep)[4];
				double limRelease = (double)((float*)cep)[5];
				if (limThreshold > -0.09)
					limThreshold = -0.09;
				if (limRelease < 0.15)
					limRelease = 0.15;
				JLimiterSetCoefficients(&kLimiter, limThreshold, limRelease, (double)mSamplingRate);
#ifdef DEBUG
				LOGI("limThreshold: %lf, limRelease: %lf", limThreshold, limRelease);
#endif
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 60)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 115)
			{
				double mBand[NUM_BANDS];
				for (int i = 0; i < NUM_BANDS; i++)
					mBand[i] = (double)((float*)cep)[4 + i];
				refreshEqBands(DSPbufferLength, mBand);
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1997)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[0][i] = (double)((float*)cep)[4 + i];
#ifdef DEBUG
						//						LOGI("bench_c0: %lf", benchmarkValue[0][i]);
#endif
					}
					isBenchData++;
				}
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1998)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[1][i] = (double)((float*)cep)[4 + i];
#ifdef DEBUG
						//						LOGI("bench_c1: %lf", benchmarkValue[1][i]);
#endif
					}
					isBenchData++;
					if (convolverReady > 0)
					{
						if (!refreshConvolver(DSPbufferLength))
						{
							if (finalImpulse)
							{
								for (int i = 0; i < impChannels; i++)
									free(finalImpulse[i]);
								free(finalImpulse);
								finalImpulse = 0;
								free(tempImpulsedouble);
								tempImpulsedouble = 0;
							}
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
						}
					}
				}
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 8)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 8888)
			{
				int32_t times = ((int32_t *)cep)[4];
				int32_t sizePerBuffer = ((int32_t *)cep)[5];
				stringLength = times * sizePerBuffer;
#ifdef DEBUG
				LOGI("Allocate %d stringLength", stringLength);
#endif
				stringEq = (char*)calloc(stringLength, sizeof(char));
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 9999)
			{
				previousimpChannels = impChannels;
				impChannels = ((int32_t *)cep)[5];
				impulseLengthActual = ((int32_t *)cep)[4] / impChannels;
				convGaindB = (double)((int32_t *)cep)[6] / 262144.0;
				if (convGaindB > 50.0)
					convGaindB = 50.0;
				int numTime2Send = ((int32_t *)cep)[7];
				tempImpulseIncoming = (float*)calloc(4096 * impChannels * numTime2Send, sizeof(float));
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16384)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12000)
			{
				memcpy(tempImpulseIncoming + (samplesInc * 4096), ((float*)cep) + 4, 4096 * sizeof(float));
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 256)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12001)
			{
#ifdef DEBUG
				LOGI("Copying string");
#endif
				memcpy(stringEq + (stringIndex * 256), ((char*)cep) + 16, 256 * sizeof(char));
				*replyData = 0;
				return 0;
			}
		}
		return -1;
	}
	return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}
void EffectDSPMain::refreshTubeAmp()
{
	if (!InitTube(&tubeP[0], 0, mSamplingRate, tubedrive, 8192, 0))
		analogModelEnable = 0;
	tubeP[1] = tubeP[0];
	rightparams2.tube = tubeP;
}
void EffectDSPMain::refreshBassLinearPhase(uint32_t DSPbufferLength, uint32_t tapsLPFIR, double bassBoostCentreFreq)
{
	double strength = (double)bassBoostStrength / 100.0;
	if (strength < 1.0)
		strength = 1.0;
	int filterLength = (int)tapsLPFIR;
	double transition = 80.0;
	if (filterLength > 4096)
		transition = 40.0;
	double freq[4] = { 0, (bassBoostCentreFreq * 2.0) / mSamplingRate, (bassBoostCentreFreq * 2.0 + transition) / mSamplingRate, 1.0 };
	double amplitude[4] = { strength, strength, 0, 0 };
	double *freqSamplImp = fir2(&filterLength, freq, amplitude, 4);
#ifdef DEBUG
	LOGI("filterLength: %d", filterLength);
	if (!freqSamplImp)
		LOGI("Pointer freqSamplImp is invalid");
#endif
	unsigned int i;
	if (!bassBoostLp)
	{
		bassBoostLp = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			bassBoostLp[i] = AllocateAutoConvolver1x1ZeroLatency(freqSamplImp, filterLength, DSPbufferLength);
		ramp = 0.4;
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolver1x1ZeroLatency(bassBoostLp[i], freqSamplImp, filterLength);
	}
	free(freqSamplImp);
#ifdef DEBUG
	LOGI("Linear phase bass boost allocate all done: total taps %d", filterLength);
#endif
	bassLpReady = 1;
}
int EffectDSPMain::refreshConvolver(uint32_t DSPbufferLength)
{
	if (!finalImpulse)
		return 0;
#ifdef DEBUG
	LOGI("refreshConvolver::IR channel count:%d, IR frame count:%d, Audio buffer size:%d", impChannels, impulseLengthActual, DSPbufferLength);
#endif
	int i;
	FreeConvolver();
	if (!convolver)
	{
		if (impChannels < 3)
		{
			convolver = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * 2);
			if (!convolver)
				return 0;
			for (i = 0; i < 2; i++)
			{
				if (impChannels == 1)
					convolver[i] = InitAutoConvolver1x1(finalImpulse[0], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
				else
					convolver[i] = InitAutoConvolver1x1(finalImpulse[i], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
			}
			fullStconvparams.conv = convolver;
			fullStconvparams.out = outputBuffer;
			if (finalImpulse)
			{
				for (i = 0; i < impChannels; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulsedouble);
				tempImpulsedouble = 0;
			}
			if (impulseLengthActual < 20000)
				convolverReady = 1;
			else
				convolverReady = 2;
#ifdef DEBUG
			LOGI("Convolver strategy used: %d", convolver[0]->methods);
#endif
		}
		else if (impChannels == 4)
		{
			fullStereoConvolver = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * 4);
			if (!fullStereoConvolver)
				return 0;
			for (i = 0; i < 4; i++)
				fullStereoConvolver[i] = InitAutoConvolver1x1(finalImpulse[i], impulseLengthActual, DSPbufferLength, convGaindB, benchmarkValue, 12);
			fullStconvparams.conv = fullStereoConvolver;
			fullStconvparams1.conv = fullStereoConvolver;
			fullStconvparams.out = tempBuf;
			fullStconvparams1.out = tempBuf;
			if (finalImpulse)
			{
				for (i = 0; i < 4; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulsedouble);
				tempImpulsedouble = 0;
			}
			if (impulseLengthActual < 8192)
				convolverReady = 3;
			else
				convolverReady = 4;
#ifdef DEBUG
			LOGI("Convolver strategy used: %d", fullStereoConvolver[0]->methods);
#endif
		}
		ramp = 0.4;
#ifdef DEBUG
		LOGI("Convolver IR allocate complete");
#endif
	}
	return 1;
}
void EffectDSPMain::refreshStereoWiden(uint32_t parameter)
{
	switch (parameter)
	{
	case 0: // A Bit
		mMatrixMCoeff = 1.0 * 0.5;
		mMatrixSCoeff = 1.2 * 0.5;
		break;
	case 1: // Slight
		mMatrixMCoeff = 0.95 * 0.5;
		mMatrixSCoeff = 1.4 * 0.5;
		break;
	case 2: // Moderate
		mMatrixMCoeff = 0.9 * 0.5;
		mMatrixSCoeff = 1.6 * 0.5;
		break;
	case 3: // High
		mMatrixMCoeff = 0.85 * 0.5;
		mMatrixSCoeff = 1.8 * 0.5;
		break;
	case 4: // Super
		mMatrixMCoeff = 0.8 * 0.5;
		mMatrixSCoeff = 2.0 * 0.5;
		break;
	}
}
void EffectDSPMain::refreshCompressor()
{
	sf_advancecomp(&compressor, mSamplingRate, pregain, threshold, knee, ratio, attack, release, 0.003, 0.09, 0.16, 0.42, 0.98, -(pregain / 1.4));
	ramp = 0.3;
}
void EffectDSPMain::refreshEqBands(uint32_t DSPbufferLength, double *bands)
{
	if (!arbEq || !xaxis || !yaxis)
		return;
#ifdef DEBUG
	LOGI("Allocating FIR Equalizer");
#endif
	double y2[NUM_BANDS];
	double workingBuf[NUM_BANDSM1]; // interpFreq or bands data length minus 1
	spline(&interpFreq[0], bands, NUM_BANDS, &y2[0], &workingBuf[0]);
	splint(&interpFreq[0], bands, &y2[0], NUM_BANDS, xaxis, yaxis, 1024, 1);
	int i;
	for (i = 0; i < 1024; i++)
		arbEq->nodes[i]->gain = yaxis[i];
	double *eqImpulseResponse = arbEq->GetFilter(arbEq, mSamplingRate);
	if (!FIREq)
	{
		FIREq = (AutoConvolver1x1**)malloc(sizeof(AutoConvolver1x1*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			FIREq[i] = AllocateAutoConvolver1x1ZeroLatency(eqImpulseResponse, eqfilterLength, DSPbufferLength);
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolver1x1ZeroLatency(FIREq[i], eqImpulseResponse, eqfilterLength);
	}
#ifdef DEBUG
	LOGI("FIR Equalizer allocate all done: total taps %d", eqfilterLength);
#endif
	eqFIRReady = 1;
}
void EffectDSPMain::refreshReverb()
{
	if (mPreset < 0 || mPreset > 18)
		mPreset = 0;
	sf_presetreverb(&myreverb, mSamplingRate, (sf_reverb_preset)mPreset);
}
void *EffectDSPMain::threadingConvF(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[0]->process(arguments->conv[0], arguments->in[0], arguments->out[0], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF1(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[1]->process(arguments->conv[1], arguments->in[0], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF2(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[3]->process(arguments->conv[3], arguments->in[1], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingTube(void *args)
{
	ptrThreadParamsTube *arguments = (ptrThreadParamsTube*)args;
	processTube(&arguments->tube[1], arguments->in[1], arguments->in[1], arguments->frameCount);
	return 0;
}
int32_t EffectDSPMain::process(audio_buffer_t *in, audio_buffer_t *out)
{
	int i, framePos, framePos2x, actualFrameCount = in->frameCount;
	int pos = inOutRWPosition;
	switch (formatFloatModeInt32Mode)
	{
	case 0:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->s16[framePos2x] * 3.051757812500000e-05;
			out->s16[framePos2x] = (int16_t)(outputBuffer[0][pos] * 32768.0);
			inputBuffer[1][pos] = (double)in->s16[++framePos2x] * 3.051757812500000e-05;
			out->s16[framePos2x] = (int16_t)(outputBuffer[1][pos] * 32768.0);
			pos++;
			if (pos == DSPbufferLength)
			{
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	case 1:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->f32[framePos2x];
			out->f32[framePos2x] = (float)outputBuffer[0][pos];
			inputBuffer[1][pos] = (double)in->f32[++framePos2x];
			out->f32[framePos2x] = (float)outputBuffer[1][pos];
			pos++;
			if (pos == DSPbufferLength)
			{
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	case 2:
		for (framePos = 0; framePos < actualFrameCount; framePos++)
		{
			framePos2x = framePos << 1;
			outputBuffer[0][pos] *= ramp;
			outputBuffer[1][pos] *= ramp;
			JLimiterProcess(&kLimiter, &outputBuffer[0][pos], &outputBuffer[1][pos]);
			if (outputBuffer[0][pos] > 1.0)
				outputBuffer[0][pos] = 1.0;
			if (outputBuffer[0][pos] < -1.0)
				outputBuffer[0][pos] = -1.0;
			if (outputBuffer[1][pos] > 1.0)
				outputBuffer[1][pos] = 1.0;
			if (outputBuffer[1][pos] < -1.0)
				outputBuffer[1][pos] = -1.0;
			inputBuffer[0][pos] = (double)in->s32[framePos2x] * 4.656612875245797e-10;
			out->s32[framePos2x] = (int32_t)(outputBuffer[0][pos] * 2147483647.0);
			inputBuffer[1][pos] = (double)in->s32[++framePos2x] * 4.656612875245797e-10;
			out->s32[framePos2x] = (int32_t)(outputBuffer[1][pos] * 2147483647.0);
			pos++;
			if (pos == DSPbufferLength)
			{
				if (bassBoostEnabled)
				{
					if (bassLpReady > 0)
					{
						bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (equalizerEnabled)
				{
					if (eqFIRReady == 1)
					{
						FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
						FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], DSPbufferLength);
					}
				}
				if (stereoWidenEnabled)
				{
					double outLR, outRL;
					for (i = 0; i < DSPbufferLength; i++)
					{
						outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
						outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
						inputBuffer[0][i] = outLR + outRL;
						inputBuffer[1][i] = outLR - outRL;
					}
				}
				if (reverbEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
						sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (convolverEnabled)
				{
					if (convolverReady == 1)
					{
						convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], DSPbufferLength);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 2)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						memcpy(inputBuffer[0], outputBuffer[0], memSize);
						memcpy(inputBuffer[1], outputBuffer[1], memSize);
					}
					else if (convolverReady == 3)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], tempBuf[1], DSPbufferLength);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
					else if (convolverReady == 4)
					{
						pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
						pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
						fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], DSPbufferLength);
						fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], DSPbufferLength);
						pthread_join(rightconv, 0);
						pthread_join(rightconv1, 0);
						for (i = 0; i < DSPbufferLength; i++)
						{
							inputBuffer[0][i] = outputBuffer[0][i] + tempBuf[0][i];
							inputBuffer[1][i] = outputBuffer[1][i] + tempBuf[1][i];
						}
					}
				}
				if (analogModelEnable)
				{
					pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
					processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], DSPbufferLength);
					pthread_join(righttube, 0);
				}
				if (bs2bEnabled == 1)
				{
					for (i = 0; i < DSPbufferLength; i++)
						BS2BProcess(&bs2b, &inputBuffer[0][i], &inputBuffer[1][i]);
				}
				if (ramp < 1.0)
					ramp += 0.05;
				if (compressionEnabled)
					sf_compressor_process(&compressor, DSPbufferLength, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1]);
				if (viperddcEnabled)
				{
					for (i = 0; i < DSPbufferLength; i++)
					{
						double sampleOutL = inputBuffer[0][i], sampleOutR = inputBuffer[1][i];
						for (int j = 0; j < usedSOSCount; j++)
							SOS_DF2_StereoProcess(sosPointer[j], sampleOutL, sampleOutR, &sampleOutL, &sampleOutR);
						outputBuffer[0][i] = sampleOutL;
						outputBuffer[1][i] = sampleOutR;
					}
				}
				else
				{
					memcpy(outputBuffer[0], inputBuffer[0], memSize);
					memcpy(outputBuffer[1], inputBuffer[1], memSize);
				}
				pos = 0;
			}
		}
		break;
	}
	inOutRWPosition = pos;
	return mEnable ? 0 : -ENODATA;
}
