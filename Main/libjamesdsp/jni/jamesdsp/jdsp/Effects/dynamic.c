#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "eel2/numericSys/codelet.h"
#include "eel2/ns-eel.h"
#include "../jdsp_header.h"
void JLimiterSetCoefficients(JamesDSPLib *jdsp, double thresholddB, double msRelease)
{
	if (msRelease < 1.5)
		msRelease = 1.5;
	jdsp->limiter.relCoef = (float)exp(-1000.0 / (msRelease * round((double)jdsp->fs)));
	jdsp->limiter.threshold = (float)pow(10.0, thresholddB / 20.0);
}
void JLimiterInit(JamesDSPLib *jdsp)
{
	jdsp->limiter.envOverThreshold = 0.0f;
}
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
static void getAsymmetricWindow(float *analysisWnd, float *synthesisWnd, int k, int m, int p, double freq_temporal)
{
	int i;
	if ((k / m) < 4)
		freq_temporal = 1.0f;
	if (freq_temporal > 9.0f)
		freq_temporal = 9.0f;
	memset(synthesisWnd, 0, k * sizeof(float));
	int n = ((k - m) << 1) + 2;
	for (i = 0; i < k - m; ++i)
		analysisWnd[i] = (float)pow(0.5 * (1.0 - cos(2.0 * M_PI * (i + 1.0) / (double)n)), freq_temporal);
	n = (m << 1) + 2;
	if (freq_temporal > 1.02)
		freq_temporal = 1.02;
	for (i = k - m; i < k; ++i)
		analysisWnd[i] = (float)pow(sqrt(0.5 * (1.0 - cos(2.0 * M_PI * ((m + i - (k - m)) + 1.0) / (double)n))), freq_temporal);
	n = m << 1;
	for (i = k - (m << 1); i < k; ++i)
		synthesisWnd[i] = (float)(0.5 * (1.0 - cos(2.0 * M_PI * (double)(i - (k - (m << 1))) / (double)n))) / analysisWnd[i];
	// Pre-shift window function
	for (i = 0; i < k - p; i++)
		synthesisWnd[i] = synthesisWnd[i + p];
}
unsigned int LLIntegerLog2M(unsigned int v)
{
	unsigned int i = 0;
	while (v > 1)
	{
		++i;
		v >>= 1;
	}
	return i;
}
unsigned LLRevBitsM(unsigned int x, unsigned int bits)
{
	unsigned int y = 0;
	while (bits--)
	{
		y = (y + y) + (x & 1);
		x >>= 1;
	}
	return y;
}
void LLbitReversalTblM(unsigned *dst, unsigned int fftLen)
{
	unsigned int bits = LLIntegerLog2M(fftLen);
	for (unsigned int i = 0; i < fftLen; ++i)
		dst[i] = LLRevBitsM(i, bits);
}
void LLsinHalfTblFloatM(float *dst, unsigned int fftLen)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / fftLen;
	for (int i = 0; i < fftLen >> 1; ++i)
		dst[i] = (float)sin(twopi_over_n * i);
}
#include "../generalDSP/spectralInterpolatorFloat.h"
static size_t choose(float *a, float *b, size_t src1, size_t src2)
{
	return (*b >= *a) ? src2 : src1;
}
static size_t fast_upper_bound4(float *vec, size_t n, float *value)
{
	size_t size = n;
	size_t low = 0;
	while (size >= 8)
	{
		size_t half = size / 2;
		size_t other_half = size - half;
		size_t probe = low + half;
		size_t other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);

		half = size / 2;
		other_half = size - half;
		probe = low + half;
		other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);

		half = size / 2;
		other_half = size - half;
		probe = low + half;
		other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);
	}
	while (size > 0) {
		size_t half = size / 2;
		size_t other_half = size - half;
		size_t probe = low + half;
		size_t other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);
	}
	return low;
}
static inline float lerp1DNoExtrapo(float val, float *x, float *y, int n)
{
	if (val <= x[0])
		return y[0];
	if (val >= x[n - 1])
		return y[n - 1];
	size_t j = fast_upper_bound4(x, n, &val);
	return ((val - x[j - 1]) / (x[j] - x[j - 1])) * (y[j] - y[j - 1]) + y[j - 1]; // Interpolation
}
void printMatrixFile(char *filename, double *mat, int rows, int cols)
{
	int i, j;
	FILE *fp = fopen(filename, "wb");
	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
			fprintf(fp, "%1.14lf,", mat[i * cols + j]);
		fprintf(fp, "\n");
	}
	fclose(fp);
}
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
void gammatoneAP(double fc, double fs, int n, double bwERB, double *bReal, double *bImag, double *aRe, double *aIm, double *delay, double *postGain)
{
	// One ERB value in Hz at this centre frequency
	double ERBHz = 24.7 + 0.108 * fc;
	// Bandwidth of the filter in Hertz
	double bwHz = bwERB * ERBHz;
	// This is when the function peaks
	*delay = (n - 1.0) / (2.0 * M_PI * bwHz);
	double btmp = pow(1.0 - exp(-2.0 * M_PI * bwHz / fs), (double)n);
	double tmpRe = -2.0 * M_PI * bwHz / fs;
	double tmpIm = -2.0 * M_PI * fc / fs;
	double atildeRe, atildeIm;
	cplxexp(tmpRe, tmpIm, &atildeRe, &atildeIm);
	double bRe, bIm;
	tmpRe = 0.0;
	tmpIm = 2 * M_PI * fc * *delay;
	cplxexp(tmpRe, tmpIm, &bRe, &bIm);
	bRe = bRe * btmp;
	bIm = bIm * btmp;
	aRe[0] = 1.0; aIm[0] = 0.0;
	for (int j = 1; j < n + 1; j++)
	{
		aRe[j] = 0.0;
		aIm[j] = 0.0;
	}
	double *tRe = (double *)malloc(n * sizeof(double));
	double *tIm = (double *)malloc(n * sizeof(double));
	// Repeat the pole n times, and expand the polynomial
	// a = poly(atilde*ones(1,n));
	for (int j = 0; j < n; j++)
	{
		for (int i = 0; i < j + 1; i++)
			complexMultiplication(atildeRe, atildeIm, aRe[i], aIm[i], &tRe[i], &tIm[i]);
		for (int i = 0; i < j + 1; i++)
		{
			aRe[i + 1] = aRe[i + 1] - tRe[i];
			aIm[i + 1] = aIm[i + 1] - tIm[i];
		}
	}
	free(tRe);
	free(tIm);
	double maxV = max(fabs(bRe), fabs(bIm));
	if (maxV < ((double)FLT_EPSILON))
	{
		*bReal = bRe / maxV * ((double)FLT_EPSILON);
		*bImag = bIm / maxV * ((double)FLT_EPSILON);
		*postGain = maxV / ((double)FLT_EPSILON);
	}
	else
	{
		*bReal = bRe;
		*bImag = bIm;
		*postGain = 1.0;
	}
}
void gammatoneAPFirstOrder(double fc, double fs, double bwERB, float *bRe, float *aRe, float *aIm)
{
	double bwHz = bwERB * (24.7 + 0.108 * fc);
	*bRe = (float)(1.0 - exp(-2.0 * M_PI * bwHz / fs));
	double atildeRe, atildeIm;
	cplxexp(-2.0 * M_PI * bwHz / fs, -2.0 * M_PI * fc / fs, &atildeRe, &atildeIm);
	*aRe = (float)(-atildeRe);
	*aIm = (float)(-atildeIm);
}
const float freq4Points[6] = { 0, 200, 1200, 3000, 10000, 24000 };
const float asnr4Points[6] = { 25, 17, 16, 15, 14, 25 };
const float wtNPoints[6] = { 0, 15, 23, 28, 40, 70 };
const float pspri = 0.9; // prior speech probability [0.5] (18)
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))
static float db2mag2(float db)
{
	return powf(10.0f, db / 20.0f);
}
static float mag2db(float mag)
{
	return 20.0f * log10f(mag + 1e-5f);
}
static inline float processfftComp(FFTCompander *comp, unsigned int idx, float logIn)
{
	float multBuf = logIn - comp->oldBuf[idx];
	if (multBuf < 0.0f)
		multBuf = 0.0f;
	multBuf *= comp->DREmult[idx];
	if (multBuf > comp->headRoomdB)
		multBuf = comp->headRoomdB;
	return db2mag2(multBuf);
}
static inline float processfftCompdB(FFTCompander *comp, unsigned int idx, float logIn)
{
	float multBuf = logIn - comp->oldBuf[idx];
	if (multBuf < 0.0f)
		multBuf = 0.0f;
	multBuf *= comp->DREmult[idx];
	if (multBuf > comp->headRoomdB)
		multBuf = comp->headRoomdB;
	return multBuf;
}
int FFTCompanderProcessSamples(FFTCompander *cm, const float *inLeft, const float *inRight, unsigned int inSampleCount, float *outL, float *outR)
{
	unsigned int outSampleCount, maxOutSampleCount, copyCount;
	outSampleCount = 0;
	maxOutSampleCount = inSampleCount;

	while (inSampleCount > 0) {
		copyCount = min(cm->mInputSamplesNeeded, inSampleCount);
		float *sampDL = &cm->mInput[0][cm->mInputPos];
		float *sampDR = &cm->mInput[1][cm->mInputPos];
		const float *max = inLeft + copyCount;
		while (inLeft < max)
		{
			*sampDL = *inLeft;
			*sampDR = *inRight;
			inLeft += 1;
			inRight += 1;
			sampDL += 1;
			sampDR += 1;
		}
		inSampleCount -= copyCount;
		cm->mInputPos = (cm->mInputPos + copyCount) & cm->minus_fftLen;
		cm->mInputSamplesNeeded -= copyCount;
		if (cm->mInputSamplesNeeded == 0)
		{
			unsigned int i;
			// copy to temporary buffer and FHT
			for (i = 0; i < cm->fftLen; ++i)
			{
				const unsigned int k = (i + cm->mInputPos) & cm->minus_fftLen;
				const float w = cm->analysisWnd[i];
				cm->mTempLBuffer[cm->mBitRev[i]] = (cm->mInput[0][k] * w);
				cm->mTempRBuffer[cm->mBitRev[i]] = (cm->mInput[1][k] * w);
			}
			cm->fft(cm->mTempLBuffer, cm->mSineTab);
			cm->fft(cm->mTempRBuffer, cm->mSineTab);
			// Spectral analysis
			int symIdx;
			float lR = cm->mTempLBuffer[0] * 2.0f;
			float rR = cm->mTempRBuffer[0] * 2.0f;
			float leftMag, rightMag, mask;
			cm->timeDomainOut[0][0] = lR;
			cm->timeDomainOut[1][0] = rR;
			unsigned int bitRevFwd, bitRevSym;
			//idxFrame++;
			unsigned int specLen = *((unsigned int *)(cm->octaveSmooth));
			float reciprocal = *((float *)(cm->octaveSmooth + sizeof(unsigned int)));
			unsigned int lpLen = *((unsigned int *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float)));
			float *lv1 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float));
			float *lv2 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float) + (lpLen + 3) * sizeof(float));
			if (!cm->noGridDownsampling)
			{
				leftMag = fabsf(lR);
				rightMag = fabsf(rR);
				cm->mag[0] = (leftMag + rightMag) * 0.5f;
				for (i = 1; i < cm->procUpTo; i++)
				{
					symIdx = cm->fftLen - i;
					bitRevFwd = cm->mBitRev[i];
					bitRevSym = cm->mBitRev[symIdx];
					lR = cm->mTempLBuffer[i] + cm->mTempLBuffer[symIdx];
					float lI = cm->mTempLBuffer[i] - cm->mTempLBuffer[symIdx];
					rR = cm->mTempRBuffer[i] + cm->mTempRBuffer[symIdx];
					float rI = cm->mTempRBuffer[i] - cm->mTempRBuffer[symIdx];
					float absV1 = fabsf(lR);
					float absV2 = fabsf(lI);
					leftMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
					absV1 = fabsf(rR);
					absV2 = fabsf(rI);
					rightMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
					cm->mag[i] = (leftMag + rightMag) * 0.5f;
				}
				ShrinkGridSpectralInterpolator(cm->octaveSmooth, cm->procUpTo, cm->mag, cm->aheight);
				for (i = 0; i < cm->smallGridSize; i++)
				{
					float magNormalized = mag2db(cm->aheight[i]);
					cm->oldBuf[i] = cm->oldBuf[i] + cm->fgt_fac * (magNormalized - cm->oldBuf[i]);
					// Log conversion
					float mask = processfftComp(cm, i, magNormalized);
					cm->finalGain[i] = mask;
				}
				for (i = 1; i < cm->procUpTo; i++)
				{
					symIdx = cm->fftLen - i;
					bitRevFwd = cm->mBitRev[i];
					bitRevSym = cm->mBitRev[symIdx];
					float val = i * reciprocal;
					if (val <= lv1[0])
						mask = cm->finalGain[0];
					else if (val >= lv1[lpLen + 3 - 1])
						mask = cm->finalGain[lpLen + 3 - 1];
					else
					{
						size_t j = fast_upper_bound4(lv1, lpLen + 3, &val);
						mask = ((val - lv1[j - 1]) * lv2[j - 1]) * (cm->finalGain[j] - cm->finalGain[j - 1]) + cm->finalGain[j - 1];
					}
					lR = cm->mTempLBuffer[i] + cm->mTempLBuffer[symIdx];
					float lI = cm->mTempLBuffer[i] - cm->mTempLBuffer[symIdx];
					rR = cm->mTempRBuffer[i] + cm->mTempRBuffer[symIdx];
					float rI = cm->mTempRBuffer[i] - cm->mTempRBuffer[symIdx];
					cm->timeDomainOut[0][bitRevFwd] = (lR + lI) * mask;
					cm->timeDomainOut[0][bitRevSym] = (lR - lI) * mask;
					cm->timeDomainOut[1][bitRevFwd] = (rR + rI) * mask;
					cm->timeDomainOut[1][bitRevSym] = (rR - rI) * mask;
				}
				//printf("\n");
			}
			else
			{
				leftMag = fabsf(cm->mTempLBuffer[0]);
				rightMag = fabsf(cm->mTempRBuffer[0]);
				float magNormalized = mag2db((leftMag + rightMag) * 0.5f);
				cm->oldBuf[0] = cm->oldBuf[0] + cm->fgt_fac * (magNormalized - cm->oldBuf[0]);
				// Log conversion
				float mask = processfftComp(cm, 0, magNormalized);
				cm->timeDomainOut[0][0] = cm->mTempLBuffer[0] * mask;
				cm->timeDomainOut[1][0] = cm->mTempRBuffer[0] * mask;
				for (i = 1; i < cm->procUpTo; i++)
				{
					symIdx = cm->fftLen - i;
					bitRevFwd = cm->mBitRev[i];
					bitRevSym = cm->mBitRev[symIdx];
					lR = cm->mTempLBuffer[i] + cm->mTempLBuffer[symIdx];
					float lI = cm->mTempLBuffer[i] - cm->mTempLBuffer[symIdx];
					rR = cm->mTempRBuffer[i] + cm->mTempRBuffer[symIdx];
					float rI = cm->mTempRBuffer[i] - cm->mTempRBuffer[symIdx];
					float absV1 = fabsf(lR);
					float absV2 = fabsf(lI);
					leftMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
					absV1 = fabsf(rR);
					absV2 = fabsf(rI);
					rightMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
					magNormalized = mag2db((leftMag + rightMag) * 0.5f);
					cm->oldBuf[i] = cm->oldBuf[i] + cm->fgt_fac * (magNormalized - cm->oldBuf[i]);
					// Log conversion
					mask = processfftComp(cm, i, magNormalized);
					cm->timeDomainOut[0][bitRevFwd] = (lR + lI) * mask;
					cm->timeDomainOut[0][bitRevSym] = (lR - lI) * mask;
					cm->timeDomainOut[1][bitRevFwd] = (rR + rI) * mask;
					cm->timeDomainOut[1][bitRevSym] = (rR - rI) * mask;
				}
			}
			for (i = cm->procUpTo; i < cm->halfLen; i++)
			{
				symIdx = cm->fftLen - i;
				bitRevFwd = cm->mBitRev[i];
				bitRevSym = cm->mBitRev[symIdx];
				lR = cm->mTempLBuffer[i] + cm->mTempLBuffer[symIdx];
				float lI = cm->mTempLBuffer[i] - cm->mTempLBuffer[symIdx];
				rR = cm->mTempRBuffer[i] + cm->mTempRBuffer[symIdx];
				float rI = cm->mTempRBuffer[i] - cm->mTempRBuffer[symIdx];
				cm->timeDomainOut[0][bitRevFwd] = lR + lI;
				cm->timeDomainOut[0][bitRevSym] = lR - lI;
				cm->timeDomainOut[1][bitRevFwd] = rR + rI;
				cm->timeDomainOut[1][bitRevSym] = rR - rI;
			}
			// reconstitute left/right channels
			cm->fft(cm->timeDomainOut[0], cm->mSineTab);
			cm->fft(cm->timeDomainOut[1], cm->mSineTab);
			cm->mOutputBufferCount++;
			if (cm->mOutputBufferCount > 2)
				continue;
			float *outBuffer = cm->mOutputBuffer[cm->mOutputBufferCount - 1];
			for (i = 0; i < cm->ovpLen; ++i)
			{
				outBuffer[0] = cm->mOverlapStage2dash[0][i] + (cm->timeDomainOut[0][i + cm->smpShift] * cm->synthesisWnd[i]);
				outBuffer[1] = cm->mOverlapStage2dash[1][i] + (cm->timeDomainOut[1][i + cm->smpShift] * cm->synthesisWnd[i]);
				outBuffer += 2;
				// overlapping
				cm->mOverlapStage2dash[0][i] = (cm->timeDomainOut[0][cm->smpShift + cm->ovpLen + i] * cm->synthesisWnd[i + cm->ovpLen]);
				cm->mOverlapStage2dash[1][i] = (cm->timeDomainOut[1][cm->smpShift + cm->ovpLen + i] * cm->synthesisWnd[i + cm->ovpLen]);
			}
			cm->mInputSamplesNeeded = cm->ovpLen;
		}
	}
	while ((cm->mOutputBufferCount > 0) && (outSampleCount < maxOutSampleCount)) {
		float *sampD = cm->mOutputBuffer[0];
		copyCount = min(cm->ovpLen - cm->mOutputReadSampleOffset, maxOutSampleCount - outSampleCount);
		float *out = sampD + (cm->mOutputReadSampleOffset * 2);
		float *max = outL + copyCount;
		while (outL < max)
		{
			*outL = *out;
			out += 1;
			*outR = *out;
			out += 1;
			outL += 1;
			outR += 1;
		}
		outSampleCount += copyCount;
		cm->mOutputReadSampleOffset += copyCount;
		if (cm->mOutputReadSampleOffset >= cm->ovpLen)
		{
			cm->mOutputBufferCount--;
			cm->mOutputReadSampleOffset = 0;
			if (cm->mOutputBufferCount > 0) {
				unsigned int i;
				float *moveToEnd = cm->mOutputBuffer[0];
				// Shift the buffers so that the current one for reading is at index 0
				for (i = 1; i < MAX_OUTPUT_BUFFERS_DRS; i++)
					cm->mOutputBuffer[i - 1] = cm->mOutputBuffer[i];
				cm->mOutputBuffer[MAX_OUTPUT_BUFFERS_DRS - 1] = 0;
				// Move the previous first buffer to the end (first null pointer)
				for (i = 0; i < MAX_OUTPUT_BUFFERS_DRS; i++)
				{
					if (!cm->mOutputBuffer[i])
					{
						cm->mOutputBuffer[i] = moveToEnd;
						break;
					}
				}
			}
		}
	}
	return outSampleCount;
}
void FFTCompanderSetavgBW(FFTCompander *cm, double avgBW)
{
	unsigned int fcLen;
	size_t virtualStructSize = EstimateMemorySpectralInterpolator(&fcLen, cm->procUpTo, avgBW, &cm->smallGridSize);
	if (fcLen)
	{
		cm->noGridDownsampling = 0;
		InitSpectralInterpolator(cm->octaveSmooth, fcLen, cm->procUpTo, avgBW);
	}
	else
	{
		cm->smallGridSize = 0;
		cm->noGridDownsampling = 1;
	}
}
static inline void filter2(float *Zre, float *Zim, float *bRe, float *aRe, float *aIm, float Xi, float *yRe, float *yIm)
{
	*yRe = *bRe * Xi + *Zre;
	*yIm = *Zim;
	*Zre = -*aRe * *yRe - -*aIm * *yIm;
	*Zim = -*aRe * *yIm + -*aIm * *yRe;
}
void HSHOSVF2nd(double gain, double overallGainDb, float *c1, float *c2, float *d0, float *d1, float *overallGain, double *trigo)
{
	double GB = pow(10.0, ((1.0 / sqrt(2.0)) * gain / 20.0));
	double G = pow(10.0, gain / 20.0);
	*overallGain = (float)pow(10.0, overallGainDb / 20.0);
	double gR;
	if (fabs(gain) < 8.0 * DBL_EPSILON)
		gR = 0;
	else
		gR = (G * G - GB * GB) / (GB * GB - 1);
	double ratOrd = sqrt(gR);
	double ratRO = sqrt(sqrt(gR));
	double gP1 = sqrt(G);
	const double si = 0.70710678118654752440084436210485;
	*c1 = (float)(2.0 - 2.0 * (trigo[1] - ratOrd) / (trigo[1] + ratOrd - 2.0 * ratRO * trigo[0] * si));
	*c2 = (float)((ratRO * trigo[3]) / (ratRO * trigo[3] - si * trigo[2]));
	*d0 = (float)((ratOrd + G * trigo[1] - 2.0 * gP1 * ratRO * trigo[0] * si) / (trigo[1] + ratOrd - 2.0 * ratRO * trigo[0] * si));
	*d1 = (float)((ratRO * trigo[3] - gP1 * si * trigo[2]) / (ratRO * trigo[3] - si * trigo[2]));
}
void HSHOSVF2ndNoOverallGain(double gain, float *c1, float *c2, float *d0, float *d1, double *trigo)
{
	double GB = pow(10.0, ((1.0 / sqrt(2.0)) * gain / 20.0));
	double G = pow(10.0, gain / 20.0);
	double gR;
	if (fabs(gain) < 8.0 * DBL_EPSILON)
		gR = 0;
	else
		gR = (G * G - GB * GB) / (GB * GB - 1);
	double ratOrd = sqrt(gR);
	double ratRO = sqrt(sqrt(gR));
	double gP1 = sqrt(G);
	const double si = 0.70710678118654752440084436210485;
	*c1 = (float)(2.0 - 2.0 * (trigo[1] - ratOrd) / (trigo[1] + ratOrd - 2.0 * ratRO * trigo[0] * si));
	*c2 = (float)((ratRO * trigo[3]) / (ratRO * trigo[3] - si * trigo[2]));
	*d0 = (float)((ratOrd + G * trigo[1] - 2.0 * gP1 * ratRO * trigo[0] * si) / (trigo[1] + ratOrd - 2.0 * ratRO * trigo[0] * si));
	*d1 = (float)((ratRO * trigo[3] - gP1 * si * trigo[2]) / (ratRO * trigo[3] - si * trigo[2]));
}
void HSHOSVF2ndPrecompute(double fs, double fc, double *trigo)
{
	double Dw = M_PI * (fc / fs - 0.5);
	double ntD = tan(Dw);
	double ntD2 = ntD * ntD;
	double stD = sin(Dw);
	double ctD = cos(Dw);
	trigo[0] = ntD;
	trigo[1] = ntD2;
	trigo[2] = stD;
	trigo[3] = ctD;
}
void FFTCompanderInit(FFTCompander *cm, float fs)
{
	unsigned int i;
	// FFT method
	const float oX[10] = { 750, 1500, 3000, 6000, 12000, 24000, 48000, 96000, 192000, 256000 };
	const float oY[10] = { 24, 48, 96, 192, 384, 768, 1536, 3072, 6144, 8192 };
	const float oX2[6] = { 0.0f, 0.25f, 0.4f, 0.5f, 0.75f, 1.0f };
	const float oY2[6] = { 9.0f, 7.0f, 4.5f, 4.0f, 2.0f, 1.0f };
	float frameLen = lerp1DNoExtrapo(fs, oX, oY, 10);
	float nextPwr2 = powf(2.0f, ceilf(logf(frameLen) / logf(2.0f)));
	float paddingRatio = map(frameLen / nextPwr2, 0.5f, 1.0f, 0.0f, 1.0f);
	float wndBeta = lerp1DNoExtrapo(paddingRatio, oX2, oY2, 6);
	cm->fftLen = (unsigned int)nextPwr2;
	if (cm->fftLen == 32)
		cm->fft = DFT32;
	else if (cm->fftLen == 64)
		cm->fft = DFT64;
	else if (cm->fftLen == 128)
		cm->fft = DFT128;
	else if (cm->fftLen == 256)
		cm->fft = DFT256;
	else if (cm->fftLen == 512)
		cm->fft = DFT512;
	else if (cm->fftLen == 1024)
		cm->fft = DFT1024;
	else if (cm->fftLen == 2048)
		cm->fft = DFT2048;
	else if (cm->fftLen == 4096)
		cm->fft = DFT4096;
	else
		cm->fft = DFT8192;
	cm->minus_fftLen = cm->fftLen - 1;
	cm->ovpLen = cm->fftLen / ANALYSIS_OVERLAP_DRS;
	cm->halfLen = (cm->fftLen >> 1) + 1;
	cm->smpShift = (cm->fftLen - (cm->ovpLen << 1));
	const float desiredProcessFreq = 24000.0f;
	unsigned int idx = (unsigned int)(desiredProcessFreq / (fs / (float)cm->fftLen)) + 1UL;
	if (idx > cm->halfLen)
		cm->procUpTo = cm->halfLen;
	else
		cm->procUpTo = idx;
	LLbitReversalTblM(cm->mBitRev, cm->fftLen);
	LLsinHalfTblFloatM(cm->mSineTab, cm->fftLen);
	for (i = 0; i < MAX_OUTPUT_BUFFERS_DRS; i++)
		cm->mOutputBuffer[i] = cm->buffer[i];
	cm->mInputSamplesNeeded = cm->ovpLen;
	cm->mInputPos = 0;
	cm->mOutputBufferCount = 0;
	cm->mOutputReadSampleOffset = 0;
	getAsymmetricWindow(cm->analysisWnd, cm->synthesisWnd, cm->fftLen, cm->ovpLen, cm->smpShift, wndBeta);
	for (i = 0; i < cm->fftLen; i++)
		cm->analysisWnd[i] *= (1.0f / cm->fftLen) * 0.5f;
	float sum = 0.0f;
	for (i = 0; i < cm->fftLen; i++)
		sum += cm->analysisWnd[i];
	FFTCompanderSetavgBW(cm, 1.2);
	cm->spectralRate = fs / (float)cm->fftLen * (float)ANALYSIS_OVERLAP_DRS;
	for (i = 0; i < HALFWNDLEN_DRS; i++)
		cm->oldBuf[i] = -20.0f;
	// Time domain method
	double fb_bwERBs = 3.0;
	double fb_lowFreqHz = 40.0;
	double fb_highFreqHz = 20000.0;
#define freq2erb(freq) (9.265 * log(1.0 + freq / (24.7 * 9.265)))
#define erb2freq(erb) (24.7 * 9.265 * (exp(erb / 9.265) - 1))
	double ERBS[DYN_BANDS_GAMMATONE];
	linspace(ERBS, DYN_BANDS_GAMMATONE, freq2erb(fb_lowFreqHz), freq2erb(fb_highFreqHz));
	memset(cm->Zre, 0, DYN_BANDS_GAMMATONE * 2 * sizeof(float));
	memset(cm->Zim, 0, DYN_BANDS_GAMMATONE * 2 * sizeof(float));
	for (i = 0; i < DYN_BANDS_GAMMATONE; i++)
	{
		cm->gmtFreq[i] = erb2freq(ERBS[i]);
		gammatoneAPFirstOrder(cm->gmtFreq[i], fs, fb_bwERBs, &cm->bRe[i], &cm->aRe[i], &cm->aIm[i]);
	}
	memcpy(cm->freq3 + 1, cm->gmtFreq, sizeof(cm->gmtFreq));
	cm->freq3[0] = 0.0f;
	cm->freq3[DYN_BANDS_GAMMATONE + 1] = fs * 0.5f;
	float *fq = cm->freq3 + 1;
	HSHOSVF2ndPrecompute(fs, fq[0], cm->trigo);
	for (unsigned int sb = 1; sb < DYN_BANDS_GAMMATONE - 1; sb++)
	{
		double designFreq = (fq[sb + 1] + fq[sb]) * 0.5;
		HSHOSVF2ndPrecompute(fs, designFreq, cm->trigo + sb * 4);
	}
	HSHOSVF2nd(0, 0, &cm->c1[0], &cm->c2[0], &cm->d0[0], &cm->d1[0], &cm->overallGain, cm->trigo);
	for (i = 1; i < DYN_BANDS_GAMMATONE - 1; i++)
		HSHOSVF2ndNoOverallGain(0, &cm->c1[i], &cm->c2[i], &cm->d0[i], &cm->d1[i], cm->trigo + i * 4);
	cm->updatePerNSmps = 2;
	cm->dsSm = 1.0f / (1.0f + cm->updatePerNSmps);
	cm->alpha = 0.5f / cm->updatePerNSmps;
}
void CompressorConstructor(JamesDSPLib *jdsp)
{
	FFTCompander *cm = (FFTCompander *)(&jdsp->comp);
	double freq[NUMPTS_DRS] = { 95.0, 200.0, 400.0, 800.0, 1600.0, 3400.0, 7500.0 };
	memcpy(cm->freq2 + 1, freq, NUMPTS_DRS * sizeof(double));
	cm->freq2[0] = 0.0;
	cm->gains2[0] = cm->gains2[1];
	cm->freq2[NUMPTS_DRS + 1] = 24000.0;
	initIerper(&cm->pch, NUMPTS_DRS + 2);
	FFTCompanderInit(cm, jdsp->fs);
}
void CompressorDestructor(JamesDSPLib *jdsp)
{
	freeIerper(&jdsp->comp.pch);
}
void CompressorEnable(JamesDSPLib *jdsp, char enable)
{
	if (jdsp->compForceRefresh)
	{
		FFTCompanderInit(&jdsp->comp, jdsp->fs);
		CompressorSetParam(jdsp, jdsp->comp.fgt_facT, jdsp->comp.granularity, jdsp->comp.tfresolution);
		CompressorSetGain(jdsp, 0, 0, 0);
	}
	if (enable)
		jdsp->compEnabled = 1;
}
void CompressorDisable(JamesDSPLib *jdsp)
{
	jdsp->compEnabled = 0;
}
void CompressorSetParam(JamesDSPLib *jdsp, float fgt_facT, int granularity, int tfresolution)
{
	FFTCompander *cm = (FFTCompander *)(&jdsp->comp);
	cm->fgt_facT = fgt_facT;
	cm->tfresolution = tfresolution;
	cm->granularity = granularity;
	if (cm->tfresolution < 2)
	{
		cm->fgt_fac = (float)(1.0 - exp(-1.0 / (cm->fgt_facT * jdsp->comp.spectralRate)));
		double avgBW;
		if (!cm->granularity)
			avgBW = 1.65;
		else if (cm->granularity == 1)
			avgBW = 1.45;
		else if (cm->granularity == 2)
			avgBW = 1.2;
		else if (cm->granularity == 3)
			avgBW = 1.15;
		else
			avgBW = 1.1;
		FFTCompanderSetavgBW(cm, avgBW);
	}
	else if (cm->tfresolution == 2)
	{
		cm->fgt_fac = (float)(1.0 - exp(-1.0 / (cm->fgt_facT * jdsp->fs)));
	}
}
void CompressorSetGain(JamesDSPLib *jdsp, double *freq, double *gains, char cpy)
{
	FFTCompander *cm = (FFTCompander *)(&jdsp->comp);
	if (cpy)
	{
		memcpy(cm->freq2 + 1, freq, NUMPTS_DRS * sizeof(double));
		memcpy(cm->gains2 + 1, gains, NUMPTS_DRS * sizeof(double));
	}
	cm->freq2[0] = 0.0;
	cm->gains2[0] = cm->gains2[1];
	cm->freq2[NUMPTS_DRS + 1] = 24000.0;
	cm->gains2[NUMPTS_DRS + 1] = cm->gains2[NUMPTS_DRS];
	makima(&cm->pch, cm->freq2, cm->gains2, NUMPTS_DRS + 2, 1, 1);
	unsigned int specLen = *((unsigned int *)(cm->octaveSmooth));
	float reciprocal = *((float *)(cm->octaveSmooth + sizeof(unsigned int)));
	unsigned int lpLen = *((unsigned int *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float)));
	float *lv1 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float));
	float *lv2 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float) + (lpLen + 3) * sizeof(float));
	if (cm->tfresolution < 2)
	{
		for (int i = 0; i < HALFWNDLEN_DRS; i++)
			cm->DREmultUniform[i] = getValueAt(&cm->pch.cb, i * jdsp->fs / cm->fftLen * 0.25);
		if (!cm->noGridDownsampling)
			ShrinkGridSpectralInterpolator(cm->octaveSmooth, cm->procUpTo, cm->DREmultUniform, cm->DREmult);
		else
			memcpy(cm->DREmult, cm->DREmultUniform, cm->procUpTo * sizeof(float));
		cm->headRoomdB = 10.0f;
		if (!cm->noGridDownsampling)
		{
			for (int i = 0; i < cm->smallGridSize; i++)
			{
				if (cm->DREmult[i] < -1.2)
					cm->DREmult[i] = -1.2;
				if (cm->DREmult[i] > 1.2)
					cm->DREmult[i] = 1.2;
				if (cm->headRoomdB < cm->DREmult[i] * 12.0f)
					cm->headRoomdB = cm->DREmult[i] * 12.0f;
			}
		}
		else
		{
			for (int i = 0; i < cm->procUpTo; i++)
			{
				if (cm->DREmult[i] < -1.2)
					cm->DREmult[i] = -1.2;
				if (cm->DREmult[i] > 1.2)
					cm->DREmult[i] = 1.2;
				if (cm->headRoomdB < cm->DREmult[i] * 12.0f)
					cm->headRoomdB = cm->DREmult[i] * 12.0f;
			}
		}
	}
	else
	{
		float avgGain = 0.0f;
		for (int i = 0; i < DYN_BANDS_GAMMATONE; i++)
		{
			cm->DREmult[i] = getValueAt(&cm->pch.cb, cm->gmtFreq[i]);
			if (cm->DREmult[i] < -1.2)
				cm->DREmult[i] = -1.2;
			if (cm->DREmult[i] > 1.2)
				cm->DREmult[i] = 1.2;
			avgGain += cm->DREmult[i];
		}
		avgGain /= DYN_BANDS_GAMMATONE;
		const float oX[11] = { -1.2, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8, 1.2 };
		const float oY[11] = {  0.05, 0.09, 0.15, 0.17, 0.2, 0.25, 0.1, 0.05, 0.008, 0.005, 0.003 };
		float rate = lerp1DNoExtrapo(avgGain, oX, oY, 11);
		if (!cm->granularity)
			rate *= 0.9f;
		else if (cm->granularity == 1)
			rate *= 0.95f;
		else if (cm->granularity == 3)
			rate *= 0.99f;
		else if (cm->granularity == 3)
			rate *= 1.0f;
		cm->alpha = rate / cm->updatePerNSmps;
		cm->headRoomdB = 10.0f;
		for (int i = 0; i < DYN_BANDS_GAMMATONE; i++)
			if (cm->headRoomdB < cm->DREmult[i] * 12.0f)
				cm->headRoomdB = cm->DREmult[i] * 12.0f;
	}
}
void CompressorProcess(JamesDSPLib *jdsp, size_t n)
{
	FFTCompander *cm = (FFTCompander *)(&jdsp->comp);
	if (cm->tfresolution < 2)
	{
		unsigned int offset = 0;
		while (offset < n)
		{
			const unsigned int processing = min(n - offset, cm->ovpLen);
			FFTCompanderProcessSamples(&jdsp->comp, jdsp->tmpBuffer[0] + offset, jdsp->tmpBuffer[1] + offset, processing, jdsp->tmpBuffer[0] + offset, jdsp->tmpBuffer[1] + offset);
			offset += processing;
		}
	}
	else
	{
		unsigned int smp, i, sb;
		float yre, yim;
		float *ZreL = cm->Zre;
		float *ZimL = cm->Zim;
		float *ZreR = ZreL + DYN_BANDS_GAMMATONE;
		float *ZimR = ZimL + DYN_BANDS_GAMMATONE;
		for (smp = 0; smp < n; smp++)
		{
			for (sb = 0; sb < DYN_BANDS_GAMMATONE; sb++)
			{
				filter2(&ZreL[sb], &ZimL[sb], &cm->bRe[sb], &cm->aRe[sb], &cm->aIm[sb], jdsp->tmpBuffer[0][smp], &yre, &yim);
				float leftMag = hypotf(yre, yim);
				filter2(&ZreR[sb], &ZimR[sb], &cm->bRe[sb], &cm->aRe[sb], &cm->aIm[sb], jdsp->tmpBuffer[1][smp], &yre, &yim);
				float rightMag = hypotf(yre, yim);
				float magNormalized = mag2db((leftMag + rightMag) * 0.5f);
				cm->oldBuf[sb] = cm->oldBuf[sb] + cm->fgt_fac * (magNormalized - cm->oldBuf[sb]);
				cm->interpolatedGain[sb + 1] = processfftCompdB(cm, sb, magNormalized);
			}
			cm->interpolatedGain[0] = cm->interpolatedGain[1];
			cm->interpolatedGain[DYN_BANDS_GAMMATONE + 1] = cm->interpolatedGain[DYN_BANDS_GAMMATONE];
			float *gn = cm->interpolatedGain + 1;
			cm->diffGain[0] = gn[0] + cm->dsSm * (cm->diffGain[0] - gn[0]);
			for (sb = 0; sb < DYN_BANDS_GAMMATONE - 1; sb++)
			{
				float diff = gn[sb + 1] - gn[sb];
				cm->diffGain[sb + 1] = diff + cm->dsSm * (cm->diffGain[sb + 1] - diff);
			}
			cm->updateIdx = (cm->updateIdx + 1) & (cm->updatePerNSmps - 1);
			if (cm->updateIdx == 0)
			{
				float c1, c2, d0, d1, unityGain;
				HSHOSVF2nd(cm->diffGain[0 + 1], cm->diffGain[0], &c1, &c2, &d0, &d1, &unityGain, cm->trigo);
				cm->d0step[0] = (d0 - cm->d0[0]) * cm->alpha;
				cm->d1step[0] = (d1 - cm->d1[0]) * cm->alpha;
				cm->c1step[0] = (c1 - cm->c1[0]) * cm->alpha;
				cm->c2step[0] = (c2 - cm->c2[0]) * cm->alpha;
				cm->overallGainstep = (unityGain - cm->overallGain) * cm->alpha;
				for (sb = 1; sb < DYN_BANDS_GAMMATONE - 1; sb++)
				{
					HSHOSVF2ndNoOverallGain(cm->diffGain[sb + 1], &c1, &c2, &d0, &d1, cm->trigo + sb * 4);
					cm->d0step[sb] = (d0 - cm->d0[sb]) * cm->alpha;
					cm->d1step[sb] = (d1 - cm->d1[sb]) * cm->alpha;
					cm->c1step[sb] = (c1 - cm->c1[sb]) * cm->alpha;
					cm->c2step[sb] = (c2 - cm->c2[sb]) * cm->alpha;
				}
			}
			for (sb = 0; sb < DYN_BANDS_GAMMATONE - 1; sb++)
			{
				cm->d0[sb] += cm->d0step[sb];
				cm->d1[sb] += cm->d1step[sb];
				cm->c1[sb] += cm->c1step[sb];
				cm->c2[sb] += cm->c2step[sb];
			}
			cm->overallGain += cm->overallGainstep;
			float x1 = jdsp->tmpBuffer[0][smp];
			float x2 = jdsp->tmpBuffer[1][smp];
			for (i = 0; i < DYN_BANDS_GAMMATONE - 1; i++)
			{
				float y1 = x1 - cm->z1_AL[i] - cm->z2_AL[i];
				x1 = cm->d0[i] * y1 + cm->d1[i] * cm->z1_AL[i] + cm->z2_AL[i];
				cm->z2_AL[i] = cm->z2_AL[i] + cm->c2[i] * cm->z1_AL[i];
				cm->z1_AL[i] = cm->z1_AL[i] + cm->c1[i] * y1;
				float y2 = x2 - cm->z1_AR[i] - cm->z2_AR[i];
				x2 = cm->d0[i] * y2 + cm->d1[i] * cm->z1_AR[i] + cm->z2_AR[i];
				cm->z2_AR[i] = cm->z2_AR[i] + cm->c2[i] * cm->z1_AR[i];
				cm->z1_AR[i] = cm->z1_AR[i] + cm->c1[i] * y2;
			}
			jdsp->tmpBuffer[0][smp] = x1 * cm->overallGain;
			jdsp->tmpBuffer[1][smp] = x2 * cm->overallGain;
		}
	}
}
