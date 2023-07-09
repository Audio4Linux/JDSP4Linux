#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "../jdsp_header.h"
void MultimodalEqualizerConstructor(JamesDSPLib *jdsp)
{
	double freqAx[NUMPTS] = { 25.0, 40.0, 63.0, 100.0, 160.0, 250.0, 400.0, 630.0, 1000.0, 1600.0, 2500.0, 4000.0, 6300.0, 10000.0, 16000.0 };
	memcpy(jdsp->mEQ.freq + 1, freqAx, NUMPTS * sizeof(double));
	jdsp->mEQ.freq[NUMPTS + 1] = 24000.0;
	memset(jdsp->mEQ.z1_AL, 0, sizeof(jdsp->mEQ.z1_AL));
	memset(jdsp->mEQ.z2_AL, 0, sizeof(jdsp->mEQ.z2_AL));
	memset(jdsp->mEQ.z1_AR, 0, sizeof(jdsp->mEQ.z1_AR));
	memset(jdsp->mEQ.z2_AR, 0, sizeof(jdsp->mEQ.z2_AR));
	initIerper(&jdsp->mEQ.pch1, NUMPTS + 2);
	initIerper(&jdsp->mEQ.pch2, NUMPTS + 2);
	jdsp->mEQ.instance.filterLen = InitArbitraryEq(&jdsp->mEQ.instance.coeffGen, 0);
	FFTConvolver2x2Init(&jdsp->mEQ.instance.convState);
	FFTConvolver2x2Init(&jdsp->mEQ.conv);
	float *kDelta = (float*)malloc(jdsp->mEQ.instance.filterLen * sizeof(float));
	memset(kDelta, 0, jdsp->mEQ.instance.filterLen * sizeof(float));
	kDelta[0] = 1.0f;
	FFTConvolver2x2LoadImpulseResponse(&jdsp->mEQ.instance.convState, (unsigned int)jdsp->blockSize, kDelta, kDelta, jdsp->mEQ.instance.filterLen);
	FFTConvolver2x2LoadImpulseResponse(&jdsp->mEQ.conv, (unsigned int)jdsp->blockSize, kDelta, kDelta, jdsp->mEQ.instance.filterLen);
	free(kDelta);
}
void MultimodalEqualizerDestructor(JamesDSPLib *jdsp)
{
	freeIerper(&jdsp->mEQ.pch1);
	freeIerper(&jdsp->mEQ.pch2);
	EqNodesFree(&jdsp->mEQ.instance.coeffGen);
	FFTConvolver2x2Free(&jdsp->mEQ.instance.convState);
	FFTConvolver2x2Free(&jdsp->mEQ.conv);
}
void HSHOResponse(double fs, double fc, unsigned int filterOrder, double gain, double overallGainDb, unsigned int queryPts, double *dispFreq, double *cplxRe, double *cplxIm)
{
	unsigned int L = filterOrder >> 1;
	double *sRe = (double *)malloc(queryPts * sizeof(double));
	double *sIm = (double *)malloc(queryPts * sizeof(double));
	double *s2Re = (double *)malloc(queryPts * sizeof(double));
	double *s2Im = (double *)malloc(queryPts * sizeof(double));
	for (unsigned int j = 0; j < queryPts; j++)
	{
		double digw = dispFreq[j] / fs * 2.0 * M_PI;
		sRe[j] = cos(digw);
		sIm[j] = sin(digw);
		s2Re[j] = sRe[j] * sRe[j] - sIm[j] * sIm[j];
		s2Im[j] = sRe[j] * sIm[j] + sIm[j] * sRe[j];
	}
	double Dw = M_PI * (0.5 - fc / fs);
	double GB = (1.0 / sqrt(2.0)) * gain;
	double G = pow(10.0, gain / 20.0);
	double overallGain = pow(10.0, overallGainDb / 20.0);
	if (G == 1.0)
		goto scalar_gain;
	GB = pow(10.0, GB / 20.0);
	double gR = (G * G - GB * GB) / (GB * GB - 1);
	double reci1Ord = 1.0 / filterOrder;
	double ratOrd = pow(gR, reci1Ord);
	double sDw = sin(Dw);
	double sDw2 = sDw * sDw;
	double cDw = cos(Dw);
	double cDw2 = cDw * cDw;
	double ratRO = pow(gR, 1.0 / (2 * filterOrder));
	double g2reO = pow(G, 2.0 * reci1Ord);
	double gP1 = pow(G, 1.0 / filterOrder);
	double t2 = 2 * g2reO * sDw2;
	double t4 = cDw2 * ratOrd;
	double t5 = 2 * cDw2 * ratOrd;
	double t8 = cDw2 * ratOrd;
	double t10 = g2reO * sDw2;
	double t11 = 2 * sDw2;
	double t12 = 2 * t8;
	double t13 = 2 * gP1 * cDw * sDw * ratRO;
	double t14 = g2reO * sDw2 + cDw2 * ratOrd;
	double t15 = 2 * cDw * sDw * ratRO;
	double t16 = sDw2 + cDw2 * ratOrd;
	for (unsigned int i = 0; i < L; i++)
	{
		double si = sin((2 * (i + 1) - 1.0) * M_PI / (2.0 * filterOrder));
		double t6 = t13 * si;
		double t1 = t14 - t6;
		double t9 = t15 * si;
		double t7 = t16 - t9;
		for (unsigned int j = 0; j < queryPts; j++)
		{
			double k1Re = (t1 - t2 * sRe[j] + t5 * sRe[j] + t10 * s2Re[j] + t4 * s2Re[j] + t6 * s2Re[j]);
			double k1Im = (-t2 * sIm[j] + t5 * sIm[j] + t10 * s2Im[j] + t4 * s2Im[j] + t6 * s2Im[j]);
			double k2Re = (t7 + s2Re[j] * sDw2 - t11 * sRe[j] + s2Re[j] * t8 + sRe[j] * t12 + s2Re[j] * t9);
			double k2Im = (s2Im[j] * sDw2 - t11 * sIm[j] + s2Im[j] * t8 + sIm[j] * t12 + s2Im[j] * t9);
			double cplx2Re, cplx2Im;
			cdivid(k1Re, k1Im, k2Re, k2Im, &cplx2Re, &cplx2Im);
			complexMultiplication(cplxRe[j], cplxIm[j], cplx2Re, cplx2Im, &cplxRe[j], &cplxIm[j]);
		}
	}
	free(sRe);
	free(sIm);
	free(s2Re);
	free(s2Im);
scalar_gain:
	for (unsigned int j = 0; j < queryPts; j++)
	{
		cplxRe[j] *= overallGain;
		cplxIm[j] *= overallGain;
	}
}
unsigned int HSHOSVF(double fs, double fc, unsigned int filterOrder, double gain, double overallGainDb, float *c1, float *c2, float *d0, float *d1, float *overallGain)
{
	unsigned int L = filterOrder >> 1;
	double Dw = M_PI * (fc / fs - 0.5);
	double GB = pow(10.0, ((1.0 / sqrt(2.0)) * gain / 20.0));
	double G = pow(10.0, gain / 20.0);
	*overallGain = (float)pow(10.0, overallGainDb / 20.0);
	if (G == 1.0)
		return 0;
	double gR = (G * G - GB * GB) / (GB * GB - 1);
	double reci1Ord = 1.0 / filterOrder;
	double ratOrd = pow(gR, reci1Ord);
	double ntD = tan(Dw);
	double ntD2 = ntD * ntD;
	double stD = sin(Dw);
	double ctD = cos(Dw);
	double ratRO = pow(gR, 1.0 / (2 * filterOrder));
	double gP1 = pow(G, 1.0 / filterOrder);
	double gP2 = pow(G, 2.0 / filterOrder);
	for (unsigned int i = 0; i < L; i++)
	{
		double si = sin((2 * (i + 1) - 1.0) * M_PI / (2.0 * filterOrder));
		c1[i] = (float)(2.0 - 2.0 * (ntD2 - ratOrd) / (ntD2 + ratOrd - 2.0 * ratRO * ntD * si));
		c2[i] = (float)((ratRO * ctD) / (ratRO * ctD - si * stD));
		d0[i] = (float)((ratOrd + gP2 * ntD2 - 2.0 * gP1 * ratRO * ntD * si) / (ntD2 + ratOrd - 2.0 * ratRO * ntD * si));
		d1[i] = (float)((ratRO * ctD - gP1 * si * stD) / (ratRO * ctD - si * stD));
	}
	return L;
}
double reverseProjectX(double pos, double MIN_FREQ, double MAX_FREQ)
{
	double minPos = log(MIN_FREQ);
	double maxPos = log(MAX_FREQ);
	return exp(pos * (maxPos - minPos) + minPos);
}
void MultimodalEqualizerAxisInterpolation(JamesDSPLib *jdsp, int interpolationMode, int operatingMode, double *freqAx, double *gaindB)
{
	memcpy(jdsp->mEQ.freq + 1, freqAx, NUMPTS * sizeof(double));
	memcpy(jdsp->mEQ.gain + 1, gaindB, NUMPTS * sizeof(double));
	for (int i = 0; i < NUMPTS; i++)
	{
		if (jdsp->mEQ.gain[i] < -64.0)
			jdsp->mEQ.gain[i] = -64.0;
		if (jdsp->mEQ.gain[i] > 64.0)
			jdsp->mEQ.gain[i] = 64.0;
	}
	jdsp->mEQ.freq[0] = 0.0;
	jdsp->mEQ.gain[0] = jdsp->mEQ.gain[1];
	jdsp->mEQ.freq[NUMPTS + 1] = 24000.0;
	jdsp->mEQ.gain[NUMPTS + 1] = jdsp->mEQ.gain[NUMPTS];
	if (!operatingMode)
	{
		float *eqFil;
		if (!interpolationMode)
		{
			pchip(&jdsp->mEQ.pch1, jdsp->mEQ.freq, jdsp->mEQ.gain, NUMPTS + 2, 1, 1);
			eqFil = InterpolatingEqMinimumPhase(&jdsp->mEQ.instance.coeffGen, (float)jdsp->fs, (void *)(&jdsp->mEQ.pch1));
		}
		else
		{
			makima(&jdsp->mEQ.pch2, jdsp->mEQ.freq, jdsp->mEQ.gain, NUMPTS + 2, 1, 1);
			eqFil = InterpolatingEqMinimumPhase(&jdsp->mEQ.instance.coeffGen, (float)jdsp->fs, (void *)(&jdsp->mEQ.pch2));
		}
		FFTConvolver2x2RefreshImpulseResponse(&jdsp->mEQ.instance.convState, &jdsp->mEQ.conv, eqFil, eqFil, jdsp->mEQ.instance.filterLen);
	}
	else
	{
		double *freq = jdsp->mEQ.freq + 1;
		double *gains = jdsp->mEQ.gain + 1;
		if (operatingMode == 1)
			jdsp->mEQ.order = 4;
		else if (operatingMode == 2)
			jdsp->mEQ.order = 6;
		else if (operatingMode == 3)
			jdsp->mEQ.order = 8;
		else if (operatingMode == 4)
			jdsp->mEQ.order = 10;
		else if (operatingMode == 5)
			jdsp->mEQ.order = 12;
		jdsp->mEQ.nSec = jdsp->mEQ.order >> 1;
		if (jdsp->mEQ.nSec > MAXSECTIONS)
		{
			jdsp->mEQ.order = MAXORDER;
			jdsp->mEQ.nSec = MAXSECTIONS;
		}
		/*double fs = 48000;
		double MIN_FREQ = 21;
		double MAX_FREQ = 20000;
		unsigned int nPts = 131072 / 2 + 1;
		double *dispFreq = (double *)malloc(nPts * sizeof(double));
		for (int i = 0; i < nPts; i++)
			dispFreq[i] = reverseProjectX(i / (double)(nPts - 1), MIN_FREQ, MAX_FREQ);
		double *cplxRe = (double *)malloc(nPts * sizeof(double));
		double *cplxIm = (double *)malloc(nPts * sizeof(double));
		for (int i = 0; i < nPts; i++)
		{
			cplxRe[i] = 1;
			cplxIm[i] = 0;
		}*/
		for (int i = 0; i < NUMPTS - 1; i++)
		{
			double dB = gains[i + 1] - gains[i];
			double designFreq;
			if (i)
				designFreq = (freq[i + 1] + freq[i]) * 0.5;
			else
				designFreq = freq[i];
			double overallGain = i == 0 ? gains[i] : 0.0;
			//HSHOResponse(jdsp->fs, designFreq, jdsp->mEQ.order, dB, overallGain, nPts, dispFreq, cplxRe, cplxIm);
			jdsp->mEQ.sec[i] = HSHOSVF(jdsp->fs, designFreq, jdsp->mEQ.order, dB, overallGain, jdsp->mEQ.c1 + i * jdsp->mEQ.nSec, jdsp->mEQ.c2 + i * jdsp->mEQ.nSec, jdsp->mEQ.d0 + i * jdsp->mEQ.nSec, jdsp->mEQ.d1 + i * jdsp->mEQ.nSec, &jdsp->mEQ.overallGain[i]);
			//printf("nSec: %d\n", jdsp->mEQ.sec[i]);
		}
	}
	jdsp->mEQ.operatingMode = operatingMode;
	jdsp->mEQ.currentInterpolationMode = interpolationMode;
}
void MultimodalEqualizerEnable(JamesDSPLib *jdsp, char enable)
{
	if (jdsp->equalizerForceRefresh)
	{
		float *eqFil;
		if (!jdsp->mEQ.currentInterpolationMode)
		{
			pchip(&jdsp->mEQ.pch1, jdsp->mEQ.freq, jdsp->mEQ.gain, NUMPTS + 2, 1, 1);
			eqFil = InterpolatingEqMinimumPhase(&jdsp->mEQ.instance.coeffGen, (float)jdsp->fs, (void *)(&jdsp->mEQ.pch1));
		}
		else
		{
			makima(&jdsp->mEQ.pch2, jdsp->mEQ.freq, jdsp->mEQ.gain, NUMPTS + 2, 1, 1);
			eqFil = InterpolatingEqMinimumPhase(&jdsp->mEQ.instance.coeffGen, (float)jdsp->fs, (void *)(&jdsp->mEQ.pch2));
		}
		FFTConvolver2x2Free(&jdsp->mEQ.instance.convState);
		FFTConvolver2x2Free(&jdsp->mEQ.conv);
		FFTConvolver2x2LoadImpulseResponse(&jdsp->mEQ.instance.convState, (unsigned int)jdsp->blockSize, eqFil, eqFil, jdsp->mEQ.instance.filterLen);
		FFTConvolver2x2LoadImpulseResponse(&jdsp->mEQ.conv, (unsigned int)jdsp->blockSize, eqFil, eqFil, jdsp->mEQ.instance.filterLen);
		jdsp->equalizerForceRefresh = 0;
	}
	if (enable)
		jdsp->equalizerEnabled = 1;
}
void MultimodalEqualizerDisable(JamesDSPLib *jdsp)
{
	jdsp->equalizerEnabled = 0;
}
void MultimodalEqualizerProcess(JamesDSPLib *jdsp, size_t n)
{
	if (!jdsp->mEQ.operatingMode)
		FFTConvolver2x2Process(&jdsp->mEQ.conv, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
	else
	{
		for (size_t smp = 0; smp < n; smp++)
		{
			float x1 = jdsp->tmpBuffer[0][smp];
			float x2 = jdsp->tmpBuffer[1][smp];
			for (int i = 0; i < NUMPTS - 1; i++)
			{
				for (char j = 0; j < jdsp->mEQ.sec[i]; j++)
				{
					float y1 = x1 - jdsp->mEQ.z1_AL[i * jdsp->mEQ.nSec + j] - jdsp->mEQ.z2_AL[i * jdsp->mEQ.nSec + j];
					x1 = jdsp->mEQ.d0[i * jdsp->mEQ.nSec + j] * y1 + jdsp->mEQ.d1[i * jdsp->mEQ.nSec + j] * jdsp->mEQ.z1_AL[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.z2_AL[i * jdsp->mEQ.nSec + j];
					jdsp->mEQ.z2_AL[i * jdsp->mEQ.nSec + j] = jdsp->mEQ.z2_AL[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.c2[i * jdsp->mEQ.nSec + j] * jdsp->mEQ.z1_AL[i * jdsp->mEQ.nSec + j];
					jdsp->mEQ.z1_AL[i * jdsp->mEQ.nSec + j] = jdsp->mEQ.z1_AL[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.c1[i * jdsp->mEQ.nSec + j] * y1;
					float y2 = x2 - jdsp->mEQ.z1_AR[i * jdsp->mEQ.nSec + j] - jdsp->mEQ.z2_AR[i * jdsp->mEQ.nSec + j];
					x2 = jdsp->mEQ.d0[i * jdsp->mEQ.nSec + j] * y2 + jdsp->mEQ.d1[i * jdsp->mEQ.nSec + j] * jdsp->mEQ.z1_AR[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.z2_AR[i * jdsp->mEQ.nSec + j];
					jdsp->mEQ.z2_AR[i * jdsp->mEQ.nSec + j] = jdsp->mEQ.z2_AR[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.c2[i * jdsp->mEQ.nSec + j] * jdsp->mEQ.z1_AR[i * jdsp->mEQ.nSec + j];
					jdsp->mEQ.z1_AR[i * jdsp->mEQ.nSec + j] = jdsp->mEQ.z1_AR[i * jdsp->mEQ.nSec + j] + jdsp->mEQ.c1[i * jdsp->mEQ.nSec + j] * y2;
				}
				x1 *= jdsp->mEQ.overallGain[i];
				x2 *= jdsp->mEQ.overallGain[i];
			}
			jdsp->tmpBuffer[0][smp] = x1;
			jdsp->tmpBuffer[1][smp] = x2;
		}
	}
}