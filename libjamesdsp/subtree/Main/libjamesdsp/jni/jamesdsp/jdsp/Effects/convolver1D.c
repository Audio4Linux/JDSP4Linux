#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include "../jdsp_header.h"
void Convolver1DEnable(JamesDSPLib *jdsp)
{
	if (jdsp->conv.process)
		jdsp->convolverEnabled = 1;
}
void Convolver1DDisable(JamesDSPLib *jdsp)
{
	jdsp->convolverEnabled = 0;
}
void Convolver1DConstructor(JamesDSPLib *jdsp)
{
	memset(&jdsp->conv, 0, sizeof(jdsp->conv));
}
void Convolver1DDestructor(JamesDSPLib *jdsp)
{
	if (jdsp->conv.conv1d2x2_S_S)
	{
		FFTConvolver2x2Free(jdsp->conv.conv1d2x2_S_S);
		free(jdsp->conv.conv1d2x2_S_S);
	}
	if (jdsp->conv.conv1d2x2_T_S)
	{
		TwoStageFFTConvolver2x2Free(jdsp->conv.conv1d2x2_T_S);
		free(jdsp->conv.conv1d2x2_T_S);
	}
	if (jdsp->conv.conv1d2x4x2_S_S)
	{
		FFTConvolver2x4x2Free(jdsp->conv.conv1d2x4x2_S_S);
		free(jdsp->conv.conv1d2x4x2_S_S);
	}
	if (jdsp->conv.conv1d2x4x2_T_S)
	{
		TwoStageFFTConvolver2x4x2Free(jdsp->conv.conv1d2x4x2_T_S);
		free(jdsp->conv.conv1d2x4x2_T_S);
	}
}
void Convolver1DProcessFFTConvolver2x2(JamesDSPLib *jdsp, size_t n)
{
	FFTConvolver2x2Process(jdsp->conv.conv1d2x2_S_S, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
}
void Convolver1DProcessTwoStageFFTConvolver2x2(JamesDSPLib *jdsp, size_t n)
{
	TwoStageFFTConvolver2x2Process(jdsp->conv.conv1d2x2_T_S, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
}
void Convolver1DProcessFFTConvolver2x4x2(JamesDSPLib *jdsp, size_t n)
{
	FFTConvolver2x4x2Process(jdsp->conv.conv1d2x4x2_S_S, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
}
void Convolver1DProcessTwoStageFFTConvolver2x4x2(JamesDSPLib *jdsp, size_t n)
{
	TwoStageFFTConvolver2x4x2Process(jdsp->conv.conv1d2x4x2_T_S, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
}
int Convolver1DLoadImpulseResponse(JamesDSPLib *jdsp, float *tempImpulseFloat, unsigned int impChannels, size_t impulseLengthActual, char updateOld)
{
	jdsp_lock(jdsp);
	Convolver1DDestructor(jdsp);
	if (updateOld)
	{
		if (jdsp->impulseResponseStorage.impulseResponse)
			free(jdsp->impulseResponseStorage.impulseResponse);
		jdsp->impulseResponseStorage.impulseResponse = (float *)malloc(impChannels * impulseLengthActual * sizeof(float));
		memcpy(jdsp->impulseResponseStorage.impulseResponse, tempImpulseFloat, impChannels * impulseLengthActual * sizeof(float));
		jdsp->impulseResponseStorage.impChannels = impChannels;
		jdsp->impulseResponseStorage.impulseLengthActual = impulseLengthActual;
	}
	Convolver1DConstructor(jdsp);
	float **finalImpulse = (float**)malloc(impChannels * sizeof(float*));
	memset(finalImpulse, 0, impChannels * sizeof(float*));
	int ret = 1;
	if (!finalImpulse)
	{
		ret = 0;
		goto bufDeleteAndUnlock;
	}
	for (unsigned int i = 0; i < impChannels; i++)
	{
		float* channelbuf = (float*)malloc(impulseLengthActual * sizeof(float));
		if (!channelbuf)
		{
			ret = 0;
			goto bufDeleteAndUnlock;
		}
		float* p = tempImpulseFloat + i;
		for (unsigned int j = 0; j < impulseLengthActual; j++)
			channelbuf[j] = p[j * impChannels];
		finalImpulse[i] = channelbuf;
	}
	unsigned int seg2Len = 0;
	if (selectConvPartitions(jdsp, impulseLengthActual, &seg2Len) == 1)
	{
		if (impChannels == 1)
		{
			jdsp->conv.conv1d2x2_S_S = (FFTConvolver2x2*)malloc(sizeof(FFTConvolver2x2));
			if (!jdsp->conv.conv1d2x2_S_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			FFTConvolver2x2Init(jdsp->conv.conv1d2x2_S_S);
			ret = FFTConvolver2x2LoadImpulseResponse(jdsp->conv.conv1d2x2_S_S, (unsigned int)jdsp->blockSize, finalImpulse[0], finalImpulse[0], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x2_S_S);
				jdsp->conv.conv1d2x2_S_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessFFTConvolver2x2;
		}
		if (impChannels == 2)
		{
			jdsp->conv.conv1d2x2_S_S = (FFTConvolver2x2*)malloc(sizeof(FFTConvolver2x2));
			if (!jdsp->conv.conv1d2x2_S_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			FFTConvolver2x2Init(jdsp->conv.conv1d2x2_S_S);
			ret = FFTConvolver2x2LoadImpulseResponse(jdsp->conv.conv1d2x2_S_S, (unsigned int)jdsp->blockSize, finalImpulse[0], finalImpulse[1], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x2_S_S);
				jdsp->conv.conv1d2x2_S_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessFFTConvolver2x2;
		}
		if (impChannels == 4)
		{
			jdsp->conv.conv1d2x4x2_S_S = (FFTConvolver2x4x2*)malloc(sizeof(FFTConvolver2x4x2));
			if (!jdsp->conv.conv1d2x4x2_S_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			FFTConvolver2x4x2Init(jdsp->conv.conv1d2x4x2_S_S);
			ret = FFTConvolver2x4x2LoadImpulseResponse(jdsp->conv.conv1d2x4x2_S_S, (unsigned int)jdsp->blockSize, finalImpulse[0], finalImpulse[1], finalImpulse[2], finalImpulse[3], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x4x2_S_S);
				jdsp->conv.conv1d2x4x2_S_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessFFTConvolver2x4x2;
		}
	}
	else
	{
		if (seg2Len < 1)
			seg2Len = 1;
		if (impChannels == 1)
		{
			jdsp->conv.conv1d2x2_T_S = (TwoStageFFTConvolver2x2*)malloc(sizeof(TwoStageFFTConvolver2x2));
			if (!jdsp->conv.conv1d2x2_T_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			TwoStageFFTConvolver2x2Init(jdsp->conv.conv1d2x2_T_S);
			ret = TwoStageFFTConvolver2x2LoadImpulseResponse(jdsp->conv.conv1d2x2_T_S, (unsigned int)jdsp->blockSize, seg2Len, finalImpulse[0], finalImpulse[0], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x2_T_S);
				jdsp->conv.conv1d2x2_T_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessTwoStageFFTConvolver2x2;
		}
		if (impChannels == 2)
		{
			jdsp->conv.conv1d2x2_T_S = (TwoStageFFTConvolver2x2*)malloc(sizeof(TwoStageFFTConvolver2x2));
			if (!jdsp->conv.conv1d2x2_T_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			TwoStageFFTConvolver2x2Init(jdsp->conv.conv1d2x2_T_S);
			ret = TwoStageFFTConvolver2x2LoadImpulseResponse(jdsp->conv.conv1d2x2_T_S, (unsigned int)jdsp->blockSize, seg2Len, finalImpulse[0], finalImpulse[1], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x2_T_S);
				jdsp->conv.conv1d2x2_T_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessTwoStageFFTConvolver2x2;
		}
		if (impChannels == 4)
		{
			jdsp->conv.conv1d2x4x2_T_S = (TwoStageFFTConvolver2x4x2*)malloc(sizeof(TwoStageFFTConvolver2x4x2));
			if (!jdsp->conv.conv1d2x4x2_T_S)
			{
				ret = 0;
				goto bufDeleteAndUnlock;
			}
			TwoStageFFTConvolver2x4x2Init(jdsp->conv.conv1d2x4x2_T_S);
			ret = TwoStageFFTConvolver2x4x2LoadImpulseResponse(jdsp->conv.conv1d2x4x2_T_S, (unsigned int)jdsp->blockSize, seg2Len, finalImpulse[0], finalImpulse[1], finalImpulse[2], finalImpulse[3], impulseLengthActual);
			if (!ret)
			{
				free(jdsp->conv.conv1d2x4x2_T_S);
				jdsp->conv.conv1d2x4x2_T_S = 0;
				goto bufDeleteAndUnlock;
			}
			jdsp->conv.process = Convolver1DProcessTwoStageFFTConvolver2x4x2;
		}
	}
bufDeleteAndUnlock:
	for (unsigned int i = 0; i < impChannels; i++)
		if (finalImpulse[i])
			free(finalImpulse[i]);
	if (finalImpulse)
		free(finalImpulse);
	jdsp_unlock(jdsp);
	if (!ret)
		return -2;
	else
		return 1;
}
