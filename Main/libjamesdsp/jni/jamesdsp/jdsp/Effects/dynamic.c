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
void getwndDBL(double *wnd, unsigned int m, unsigned int n, char *mode)
{
	unsigned int i;
	double x;
	if (!strcmp(mode, "hann"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (double)(0.5 - 0.5 * cos(2 * M_PI * x));
		}
	}
	else if (!strcmp(mode, "hamming"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (double)(0.54 - 0.46 * cos(2 * M_PI * x));
		}
	}
	else if (!strcmp(mode, "blackman"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (double)(0.42 - 0.5 * cos(2 * M_PI * x) + 0.08 * cos(4 * M_PI * x));
		}
	}
	else if (!strcmp(mode, "flattop"))
	{
		double a0 = 0.21557895;
		double a1 = 0.41663158;
		double a2 = 0.277263158;
		double a3 = 0.083578947;
		double a4 = 0.006947368;
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (double)(a0 - a1 * cos(2 * M_PI * x) + a2 * cos(4 * M_PI * x) - a3 * cos(6 * M_PI * x) + a4 * cos(8 * M_PI * x));
		}
	}
}
void genWndDBL(double *wnd, unsigned int N, char *type)
{
	unsigned int plus1 = N + 1;
	unsigned int half;
	unsigned int i;
	if (plus1 % 2 == 0)
	{
		half = plus1 / 2;
		getwndDBL(wnd, half, plus1, type);
		for (i = 0; i < half - 1; i++)
			wnd[i + half] = wnd[half - i - 1];
	}
	else
	{
		half = (plus1 + 1) / 2;
		getwndDBL(wnd, half, plus1, type);
		for (i = 0; i < half - 2; i++)
			wnd[i + half] = wnd[half - i - 2];
	}
}
#include "info.h"
static inline void conjugatePadFilteringDepad(FFTCompander *cm)
{
	unsigned int i;
	for (i = 0; i < cm->prepad; ++i)
	{
		cm->specHannReal[0][cm->prepad - i - 1] = cm->specHannReal[0][cm->prepad + i + 1];
		cm->specHannImag[0][cm->prepad - i - 1] = -cm->specHannImag[0][cm->prepad + i + 1];
		cm->specHannReal[1][cm->prepad - i - 1] = cm->specHannReal[1][cm->prepad + i + 1];
		cm->specHannImag[1][cm->prepad - i - 1] = -cm->specHannImag[1][cm->prepad + i + 1];
	}
	for (i = 0; i < cm->pospad - 1; ++i)
	{
		cm->specHannReal[0][cm->prepad + cm->halfLen + cm->pospad - i - 2] = cm->specHannReal[0][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannImag[0][cm->prepad + cm->halfLen + cm->pospad - i - 2] = -cm->specHannImag[0][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannReal[1][cm->prepad + cm->halfLen + cm->pospad - i - 2] = cm->specHannReal[1][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannImag[1][cm->prepad + cm->halfLen + cm->pospad - i - 2] = -cm->specHannImag[1][cm->prepad + cm->halfLen - cm->pospad + i];
	}
	// LTV Gaussian
	cplx z1[2], z2[2], o[2], st[2];
	z1[0].real = z1[0].imag = 0;
	z2[0].real = z2[0].imag = 0;
	z1[1].real = z1[1].imag = 0;
	z2[1].real = z2[1].imag = 0;
	for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
	{
		o[0].real = cm->specHannReal[0][i] - z1[0].real - z2[0].real;
		o[0].imag = cm->specHannImag[0][i] - z1[0].imag - z2[0].imag;
		st[0].real = cm->gauss_c2[i] * z1[0].real;
		st[0].imag = cm->gauss_c2[i] * z1[0].imag;
		cm->tmp[0][i].real = cm->gauss_b[i] * o[0].real + 2 * st[0].real + z2[0].real;
		cm->tmp[0][i].imag = cm->gauss_b[i] * o[0].imag + 2 * st[0].imag + z2[0].imag;
		z2[0].real = z2[0].real + st[0].real;
		z2[0].imag = z2[0].imag + st[0].imag;
		z1[0].real = z1[0].real + cm->gauss_c1[i] * o[0].real;
		z1[0].imag = z1[0].imag + cm->gauss_c1[i] * o[0].imag;
		o[1].real = cm->specHannReal[1][i] - z1[1].real - z2[1].real;
		o[1].imag = cm->specHannImag[1][i] - z1[1].imag - z2[1].imag;
		st[1].real = cm->gauss_c2[i] * z1[1].real;
		st[1].imag = cm->gauss_c2[i] * z1[1].imag;
		cm->tmp[1][i].real = cm->gauss_b[i] * o[1].real + 2 * st[1].real + z2[1].real;
		cm->tmp[1][i].imag = cm->gauss_b[i] * o[1].imag + 2 * st[1].imag + z2[1].imag;
		z2[1].real = z2[1].real + st[1].real;
		z2[1].imag = z2[1].imag + st[1].imag;
		z1[1].real = z1[1].real + cm->gauss_c1[i] * o[1].real;
		z1[1].imag = z1[1].imag + cm->gauss_c1[i] * o[1].imag;
	}
	z1[0].real = z1[0].imag = 0;
	z2[0].real = z2[0].imag = 0;
	z1[1].real = z1[1].imag = 0;
	z2[1].real = z2[1].imag = 0;
	for (i = cm->prepad + cm->halfLen + cm->pospad - 1; i-- > 0; )
	{
		o[0].real = cm->tmp[0][i].real - z1[0].real - z2[0].real;
		o[0].imag = cm->tmp[0][i].imag - z1[0].imag - z2[0].imag;
		st[0].real = cm->gauss_c2[i] * z1[0].real;
		st[0].imag = cm->gauss_c2[i] * z1[0].imag;
		cm->specHannReal[0][i] = cm->gauss_b[i] * o[0].real + 2 * st[0].real + z2[0].real;
		cm->specHannImag[0][i] = cm->gauss_b[i] * o[0].imag + 2 * st[0].imag + z2[0].imag;
		z2[0].real = z2[0].real + st[0].real;
		z2[0].imag = z2[0].imag + st[0].imag;
		z1[0].real = z1[0].real + cm->gauss_c1[i] * o[0].real;
		z1[0].imag = z1[0].imag + cm->gauss_c1[i] * o[0].imag;
		o[1].real = cm->tmp[1][i].real - z1[1].real - z2[1].real;
		o[1].imag = cm->tmp[1][i].imag - z1[1].imag - z2[1].imag;
		st[1].real = cm->gauss_c2[i] * z1[1].real;
		st[1].imag = cm->gauss_c2[i] * z1[1].imag;
		cm->specHannReal[1][i] = cm->gauss_b[i] * o[1].real + 2 * st[1].real + z2[1].real;
		cm->specHannImag[1][i] = cm->gauss_b[i] * o[1].imag + 2 * st[1].imag + z2[1].imag;
		z2[1].real = z2[1].real + st[1].real;
		z2[1].imag = z2[1].imag + st[1].imag;
		z1[1].real = z1[1].real + cm->gauss_c1[i] * o[1].real;
		z1[1].imag = z1[1].imag + cm->gauss_c1[i] * o[1].imag;
	}
	//for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; ++i)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->specHannReal[i], cm->specHannImag[i]);
	for (i = cm->prepad; i < cm->prepad + cm->halfLen; ++i)
	{
		cm->specHannReal[0][i - cm->prepad] = cm->specHannReal[0][i] * cm->corrF[i - cm->prepad];
		cm->specHannImag[0][i - cm->prepad] = cm->specHannImag[0][i] * cm->corrF[i - cm->prepad];
		cm->specHannReal[1][i - cm->prepad] = cm->specHannReal[1][i] * cm->corrF[i - cm->prepad];
		cm->specHannImag[1][i - cm->prepad] = cm->specHannImag[1][i] * cm->corrF[i - cm->prepad];
	}
	cm->specHannImag[0][0] = cm->specHannImag[0][cm->halfLen - 1] = 0;
	cm->specHannImag[1][0] = cm->specHannImag[1][cm->halfLen - 1] = 0;
}
FILE *dbg = 0;
//int ifa = 0;
void CWTFrameInversion(FFTCompander *cm)
{
	//ifa++;
	unsigned int i, j;
	// Forward transform
	for (i = 0; i < cm->fftLen; ++i)
	{
		const unsigned int k = (i + cm->mInputPos) & cm->minus_fftLen;
		//fwrite(&cm->mInput[0][k], sizeof(float), 1, dbg);
		cm->fftBuf[0][cm->bitrevfftshift[i]] = cm->mInput[0][k];
		cm->fftBuf[1][cm->bitrevfftshift[i]] = cm->mInput[1][k];
	}
	cm->fft(cm->fftBuf[0], cm->mSineTab);
	cm->fft(cm->fftBuf[1], cm->mSineTab);
	int symIdx;
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		cm->real[0][i] = cm->fftBuf[0][i] + cm->fftBuf[0][symIdx];
		cm->imag[0][i] = -(cm->fftBuf[0][i] - cm->fftBuf[0][symIdx]);
		cm->real[1][i] = cm->fftBuf[1][i] + cm->fftBuf[1][symIdx];
		cm->imag[1][i] = -(cm->fftBuf[1][i] - cm->fftBuf[1][symIdx]);
	}
	cm->real[0][0] = cm->fftBuf[0][0] * 2.0f;
	cm->imag[0][0] = 0;
	cm->real[1][0] = cm->fftBuf[1][0] * 2.0f;
	cm->imag[1][0] = 0;
	//for (i = 0; i < cm->halfLen; ++i)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->real[i], cm->imag[i]);
	// Hann windowing
	cm->specHannReal[0][cm->prepad + 0] = (cm->real[0][0] * 2.0f + cm->real[0][1] + cm->real[0][1]) / 8.0f; // DC
	cm->specHannImag[0][cm->prepad + 0] = 0.0; // DC
	cm->specHannReal[1][cm->prepad + 0] = (cm->real[1][0] * 2.0f + cm->real[1][1] + cm->real[1][1]) / 8.0f; // DC
	cm->specHannImag[1][cm->prepad + 0] = 0.0; // DC
	for (i = 1; i < cm->halfLen - 1; i++)
	{
		cm->specHannReal[0][cm->prepad + i] = (cm->real[0][i] * 2.0f + cm->real[0][i - 1] + cm->real[0][i + 1]) / 8.0f;
		cm->specHannImag[0][cm->prepad + i] = (cm->imag[0][i] * 2.0f + cm->imag[0][i - 1] + cm->imag[0][i + 1]) / 8.0f;
		cm->specHannReal[1][cm->prepad + i] = (cm->real[1][i] * 2.0f + cm->real[1][i - 1] + cm->real[1][i + 1]) / 8.0f;
		cm->specHannImag[1][cm->prepad + i] = (cm->imag[1][i] * 2.0f + cm->imag[1][i - 1] + cm->imag[1][i + 1]) / 8.0f;
	}
	cm->specHannReal[0][cm->prepad + cm->halfLen - 1] = (cm->real[0][cm->halfLen - 1] * 2.0f + cm->real[0][cm->halfLen - 2] + cm->real[0][cm->halfLen - 2]) / 8.0f; // Nyquist
	cm->specHannImag[0][cm->prepad + cm->halfLen - 1] = 0.0f;
	cm->specHannReal[1][cm->prepad + cm->halfLen - 1] = (cm->real[1][cm->halfLen - 1] * 2.0f + cm->real[1][cm->halfLen - 2] + cm->real[1][cm->halfLen - 2]) / 8.0f; // Nyquist
	cm->specHannImag[1][cm->prepad + cm->halfLen - 1] = 0.0f;
	conjugatePadFilteringDepad(cm);
	// Spectral analysis
	float leftMag, rightMag, mask;
	//idxFrame++;
	unsigned int specLen = *((unsigned int *)(cm->octaveSmooth));
	float reciprocal = *((float *)(cm->octaveSmooth + sizeof(unsigned int)));
	unsigned int lpLen = *((unsigned int *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float)));
	float *lv1 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float));
	float *lv2 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float) + (lpLen + 3) * sizeof(float));
	if (!cm->noGridDownsampling)
	{
		for (i = 0; i < cm->procUpTo; i++)
		{
			float lR = cm->specHannReal[0][i];
			float lI = cm->specHannImag[0][i];
			float rR = cm->specHannReal[1][i];
			float rI = cm->specHannImag[1][i];
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
		for (i = 0; i < cm->procUpTo; i++)
		{
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
			cm->specHannReal[0][i] = cm->specHannReal[0][i] * mask;
			cm->specHannImag[0][i] = cm->specHannImag[0][i] * mask;
			cm->specHannReal[1][i] = cm->specHannReal[1][i] * mask;
			cm->specHannImag[1][i] = cm->specHannImag[1][i] * mask;
		}
		//printf("\n");
	}
	else
	{
		for (i = 0; i < cm->procUpTo; i++)
		{
			float lR = cm->specHannReal[0][i];
			float lI = cm->specHannImag[0][i];
			float rR = cm->specHannReal[1][i];
			float rI = cm->specHannImag[1][i];
			float absV1 = fabsf(lR);
			float absV2 = fabsf(lI);
			leftMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
			absV1 = fabsf(rR);
			absV2 = fabsf(rI);
			rightMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
			float magNormalized = mag2db((leftMag + rightMag) * 0.5f);
			cm->oldBuf[i] = cm->oldBuf[i] + cm->fgt_fac * (magNormalized - cm->oldBuf[i]);
			// Log conversion
			mask = processfftComp(cm, i, magNormalized);
			cm->specHannReal[0][i] = cm->specHannReal[0][i] * mask;
			cm->specHannImag[0][i] = cm->specHannImag[0][i] * mask;
			cm->specHannReal[1][i] = cm->specHannReal[1][i] * mask;
			cm->specHannImag[1][i] = cm->specHannImag[1][i] * mask;
		}
	}
	cm->imag[0][0] = cm->imag[1][0] = 0;
	//for (i = 0; i < cm->halfLen; ++i)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->specHannReal[i], cm->specHannImag[i]);
	//fwrite(cm->specHannReal[1], sizeof(float), cm->halfLen, dbg);
	//fwrite(cm->specHannImag[1], sizeof(float), cm->halfLen, dbg);
	// Inverse transform
	cm->real[0][0] = (cm->specHannReal[0][0] * 2.0f + cm->specHannReal[0][1] + cm->specHannReal[0][1]) / 4.0f; // DC
	cm->imag[0][0] = 0.0; // DC
	cm->real[1][0] = (cm->specHannReal[1][0] * 2.0f + cm->specHannReal[1][1] + cm->specHannReal[1][1]) / 4.0f; // DC
	cm->imag[1][0] = 0.0; // DC
	for (i = 1; i < cm->halfLen - 1; i++)
	{
		cm->real[0][i] = (cm->specHannReal[0][i] * 2.0f + cm->specHannReal[0][i - 1] + cm->specHannReal[0][i + 1]) / 4.0f;
		cm->imag[0][i] = (cm->specHannImag[0][i] * 2.0f + cm->specHannImag[0][i - 1] + cm->specHannImag[0][i + 1]) / 4.0f;
		cm->real[1][i] = (cm->specHannReal[1][i] * 2.0f + cm->specHannReal[1][i - 1] + cm->specHannReal[1][i + 1]) / 4.0f;
		cm->imag[1][i] = (cm->specHannImag[1][i] * 2.0f + cm->specHannImag[1][i - 1] + cm->specHannImag[1][i + 1]) / 4.0f;
	}
	cm->real[0][cm->halfLen - 1] = (cm->specHannReal[0][cm->halfLen - 1] * 2.0f + cm->specHannReal[0][cm->halfLen - 2] + cm->specHannReal[0][cm->halfLen - 2]) / 4.0f; // Nyquist
	cm->imag[0][cm->halfLen - 1] = 0.0;
	cm->real[1][cm->halfLen - 1] = (cm->specHannReal[1][cm->halfLen - 1] * 2.0f + cm->specHannReal[1][cm->halfLen - 2] + cm->specHannReal[1][cm->halfLen - 2]) / 4.0f; // Nyquist
	cm->imag[1][cm->halfLen - 1] = 0.0;
	cm->fftBuf[0][0] = cm->real[0][0];
	cm->fftBuf[1][0] = cm->real[1][0];
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		unsigned int bitRevFwd = cm->mBitRev[i];
		unsigned int bitRevSym = cm->mBitRev[symIdx];
		cm->fftBuf[0][bitRevFwd] = cm->real[0][i] + (-cm->imag[0][i]);
		cm->fftBuf[0][bitRevSym] = cm->real[0][i] - (-cm->imag[0][i]);
		cm->fftBuf[1][bitRevFwd] = cm->real[1][i] + (-cm->imag[1][i]);
		cm->fftBuf[1][bitRevSym] = cm->real[1][i] - (-cm->imag[1][i]);
	}
	//for (i = 0; i < cm->halfLen; ++i)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->real[i], cm->imag[i]);
	cm->fft(cm->fftBuf[0], cm->mSineTab);
	cm->fft(cm->fftBuf[1], cm->mSineTab);
	for (i = 0; i < cm->fftLen; ++i)
	{
		cm->getbackCorrectedToSpectrum1[0][cm->mBitRev[i]] = cm->fftBuf[0][i] * prec_correctionWnd[i];
		cm->getbackCorrectedToSpectrum1[1][cm->mBitRev[i]] = cm->fftBuf[1][i] * prec_correctionWnd[i];
	}
	cm->fft(cm->getbackCorrectedToSpectrum1[0], cm->mSineTab);
	cm->fft(cm->getbackCorrectedToSpectrum1[1], cm->mSineTab);
	float getbackCorrectedToSpectrum1Re1 = cm->getbackCorrectedToSpectrum1[0][0] / cm->fftLen;
	cm->specHannReal[0][cm->prepad + 0] = cm->real[0][0] * prec_wndCorrectionWeightingLF[0] + getbackCorrectedToSpectrum1Re1 * prec_wndCorrectionWeightingHF[0];
	cm->specHannImag[0][cm->prepad + 0] = 0;
	float getbackCorrectedToSpectrum1Re2 = cm->getbackCorrectedToSpectrum1[1][0] / cm->fftLen;
	cm->specHannReal[1][cm->prepad + 0] = cm->real[1][0] * prec_wndCorrectionWeightingLF[0] + getbackCorrectedToSpectrum1Re2 * prec_wndCorrectionWeightingHF[0];
	cm->specHannImag[1][cm->prepad + 0] = 0;
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		getbackCorrectedToSpectrum1Re1 = (cm->getbackCorrectedToSpectrum1[0][i] + cm->getbackCorrectedToSpectrum1[0][symIdx]) * cm->scalarGain;
		float getbackCorrectedToSpectrum1Im1 = (-(cm->getbackCorrectedToSpectrum1[0][i] - cm->getbackCorrectedToSpectrum1[0][symIdx])) * cm->scalarGain;
		cm->specHannReal[0][cm->prepad + i] = cm->real[0][i] * prec_wndCorrectionWeightingLF[i] + getbackCorrectedToSpectrum1Re1 * prec_wndCorrectionWeightingHF[i];
		cm->specHannImag[0][cm->prepad + i] = cm->imag[0][i] * prec_wndCorrectionWeightingLF[i] + getbackCorrectedToSpectrum1Im1 * prec_wndCorrectionWeightingHF[i];
		getbackCorrectedToSpectrum1Re2 = (cm->getbackCorrectedToSpectrum1[1][i] + cm->getbackCorrectedToSpectrum1[1][symIdx]) * cm->scalarGain;
		float getbackCorrectedToSpectrum1Im2 = (-(cm->getbackCorrectedToSpectrum1[1][i] - cm->getbackCorrectedToSpectrum1[1][symIdx])) * cm->scalarGain;
		cm->specHannReal[1][cm->prepad + i] = cm->real[1][i] * prec_wndCorrectionWeightingLF[i] + getbackCorrectedToSpectrum1Re2 * prec_wndCorrectionWeightingHF[i];
		cm->specHannImag[1][cm->prepad + i] = cm->imag[1][i] * prec_wndCorrectionWeightingLF[i] + getbackCorrectedToSpectrum1Im2 * prec_wndCorrectionWeightingHF[i];
		//printf("%d %1.14lf %1.14lf\n", i + 1, re, im);
	}
	// Conjugate pad
	for (i = 0; i < cm->prepad; ++i)
	{
		cm->specHannReal[0][cm->prepad - i - 1] = cm->specHannReal[0][cm->prepad + i + 1];
		cm->specHannImag[0][cm->prepad - i - 1] = -cm->specHannImag[0][cm->prepad + i + 1];
		cm->specHannReal[1][cm->prepad - i - 1] = cm->specHannReal[1][cm->prepad + i + 1];
		cm->specHannImag[1][cm->prepad - i - 1] = -cm->specHannImag[1][cm->prepad + i + 1];
	}
	for (i = 0; i < cm->pospad - 1; ++i)
	{
		cm->specHannReal[0][cm->prepad + cm->halfLen + cm->pospad - i - 2] = cm->specHannReal[0][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannImag[0][cm->prepad + cm->halfLen + cm->pospad - i - 2] = -cm->specHannImag[0][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannReal[1][cm->prepad + cm->halfLen + cm->pospad - i - 2] = cm->specHannReal[1][cm->prepad + cm->halfLen - cm->pospad + i];
		cm->specHannImag[1][cm->prepad + cm->halfLen + cm->pospad - i - 2] = -cm->specHannImag[1][cm->prepad + cm->halfLen - cm->pospad + i];
	}
	//for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->specHannReal[i], cm->specHannImag[i]);
	unsigned int ptrAcc = 0;
	for (i = 0; i < cm->halfLen; i++)
	{
		unsigned int numMultiplications = prec_mtxStrides[i];
		float sumRe[2] = { 0 };
		float sumIm[2] = { 0 };
		for (j = 0; j < numMultiplications; j++)
		{
			unsigned int readIdx = ptrAcc + j;
			sumRe[0] += cm->specHannReal[0][prec_mtxElementPos[readIdx]] * (float)prec_correctionMatrix[readIdx];
			sumIm[0] += cm->specHannImag[0][prec_mtxElementPos[readIdx]] * (float)prec_correctionMatrix[readIdx];
			sumRe[1] += cm->specHannReal[1][prec_mtxElementPos[readIdx]] * (float)prec_correctionMatrix[readIdx];
			sumIm[1] += cm->specHannImag[1][prec_mtxElementPos[readIdx]] * (float)prec_correctionMatrix[readIdx];
			//printf("%d, ", readIdx);
		}
		cm->real[0][i] = sumRe[0];
		cm->imag[0][i] = sumIm[0];
		cm->real[1][i] = sumRe[1];
		cm->imag[1][i] = sumIm[1];
		ptrAcc += numMultiplications;
	}
	cm->fftBuf[0][0] = cm->real[0][0] / cm->fftLen;
	cm->fftBuf[1][0] = cm->real[1][0] / cm->fftLen;
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		unsigned int bitRevFwd = cm->mBitRev[i];
		unsigned int bitRevSym = cm->mBitRev[symIdx];
		cm->fftBuf[0][bitRevFwd] = (cm->real[0][i] + (-cm->imag[0][i])) / cm->fftLen;
		cm->fftBuf[0][bitRevSym] = (cm->real[0][i] - (-cm->imag[0][i])) / cm->fftLen;
		cm->fftBuf[1][bitRevFwd] = (cm->real[1][i] + (-cm->imag[1][i])) / cm->fftLen;
		cm->fftBuf[1][bitRevSym] = (cm->real[1][i] - (-cm->imag[1][i])) / cm->fftLen;
	}
	cm->fft(cm->fftBuf[0], cm->mSineTab);
	cm->fft(cm->fftBuf[1], cm->mSineTab);
	//fwrite(cm->fftBuf, sizeof(float), cm->fftLen, dbg);
	cm->mOutputBufferCount++;
	float *outBuffer = cm->mOutputBuffer[cm->mOutputBufferCount - 1];
	unsigned int currentBlockIndex, nextBlockIndex, blockOffset;
	for (i = 0; i < cm->ovpLen; ++i)
	{
		outBuffer[0] = cm->mOverlapStage2dash[0][0][i] + cm->fftBuf[0][i];
		outBuffer[1] = cm->mOverlapStage2dash[1][0][i] + cm->fftBuf[1][i];
		outBuffer += 2;
		// overlapping
		currentBlockIndex = 0;
		nextBlockIndex = 1;
		blockOffset = cm->ovpLen;
		while (nextBlockIndex < cm->ovpCount - 1) {
			cm->mOverlapStage2dash[0][currentBlockIndex][i] = cm->mOverlapStage2dash[0][nextBlockIndex][i] + cm->fftBuf[0][blockOffset + i];
			cm->mOverlapStage2dash[1][currentBlockIndex][i] = cm->mOverlapStage2dash[1][nextBlockIndex][i] + cm->fftBuf[1][blockOffset + i];
			currentBlockIndex++;
			nextBlockIndex++;
			blockOffset += cm->ovpLen;
		}
		cm->mOverlapStage2dash[0][currentBlockIndex][i] = cm->fftBuf[0][blockOffset + i];
		cm->mOverlapStage2dash[1][currentBlockIndex][i] = cm->fftBuf[1][blockOffset + i];
	}
	cm->mInputSamplesNeeded = cm->ovpLen;
}
void CWTFrameSTFTInversion(FFTCompander *cm)
{
	//ifa++;
	unsigned int i, j;
	// Forward transform
	for (i = 0; i < cm->fftLen; ++i)
	{
		const unsigned int k = (i + cm->mInputPos) & cm->minus_fftLen;
		cm->fftBuf[0][cm->bitrevfftshift[i]] = cm->mInput[0][k] * cm->analysisWnd[i];
		cm->fftBuf[1][cm->bitrevfftshift[i]] = cm->mInput[1][k] * cm->analysisWnd[i];
	}
	cm->fft(cm->fftBuf[0], cm->mSineTab);
	cm->fft(cm->fftBuf[1], cm->mSineTab);
	int symIdx;
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		cm->real[0][i] = cm->fftBuf[0][i] + cm->fftBuf[0][symIdx];
		cm->imag[0][i] = -(cm->fftBuf[0][i] - cm->fftBuf[0][symIdx]);
		cm->real[1][i] = cm->fftBuf[1][i] + cm->fftBuf[1][symIdx];
		cm->imag[1][i] = -(cm->fftBuf[1][i] - cm->fftBuf[1][symIdx]);
	}
	cm->real[0][0] = cm->fftBuf[0][0] * 2.0f;
	cm->imag[0][0] = 0;
	cm->real[1][0] = cm->fftBuf[1][0] * 2.0f;
	cm->imag[1][0] = 0;
	//for (i = 0; i < cm->halfLen; ++i)
	//	printf("%d %1.14lf %1.14lf\n", i + 1, cm->real[i], cm->imag[i]);
	// Hann windowing
	memcpy(cm->specHannReal[0] + cm->prepad, cm->real[0], cm->halfLen * sizeof(float));
	memcpy(cm->specHannImag[0] + cm->prepad, cm->imag[0], cm->halfLen * sizeof(float));
	memcpy(cm->specHannReal[1] + cm->prepad, cm->real[1], cm->halfLen * sizeof(float));
	memcpy(cm->specHannImag[1] + cm->prepad, cm->imag[1], cm->halfLen * sizeof(float));
	conjugatePadFilteringDepad(cm);
	//fwrite(cm->specHannReal[1], sizeof(float), cm->halfLen, dbg);
	//fwrite(cm->specHannImag[1], sizeof(float), cm->halfLen, dbg);


	// Spectral analysis
	float leftMag, rightMag, mask;
	unsigned int bitRevFwd, bitRevSym;
	//idxFrame++;
	unsigned int specLen = *((unsigned int *)(cm->octaveSmooth));
	float reciprocal = *((float *)(cm->octaveSmooth + sizeof(unsigned int)));
	unsigned int lpLen = *((unsigned int *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float)));
	float *lv1 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float));
	float *lv2 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float) + (lpLen + 3) * sizeof(float));
	if (!cm->noGridDownsampling)
	{
		for (i = 0; i < cm->procUpTo; i++)
		{
			float lR = cm->specHannReal[0][i];
			float lI = cm->specHannImag[0][i];
			float rR = cm->specHannReal[1][i];
			float rI = cm->specHannImag[1][i];
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
		for (i = 0; i < cm->procUpTo; i++)
		{
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
			cm->real[0][i] = cm->real[0][i] * mask;
			cm->imag[0][i] = cm->imag[0][i] * mask;
			cm->real[1][i] = cm->real[1][i] * mask;
			cm->imag[1][i] = cm->imag[1][i] * mask;
		}
		//printf("\n");
	}
	else
	{
		for (i = 0; i < cm->procUpTo; i++)
		{
			float lR = cm->specHannReal[0][i];
			float lI = cm->specHannImag[0][i];
			float rR = cm->specHannReal[1][i];
			float rI = cm->specHannImag[1][i];
			float absV1 = fabsf(lR);
			float absV2 = fabsf(lI);
			leftMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
			absV1 = fabsf(rR);
			absV2 = fabsf(rI);
			rightMag = max((127.0f / 128.0f) * max(absV1, absV2) + (3.0f / 16.0f) * min(absV1, absV2), (27.0f / 32.0f) * max(absV1, absV2) + (71.0f / 128.0f) * min(absV1, absV2));
			float magNormalized = mag2db((leftMag + rightMag) * 0.5f);
			cm->oldBuf[i] = cm->oldBuf[i] + cm->fgt_fac * (magNormalized - cm->oldBuf[i]);
			// Log conversion
			mask = processfftComp(cm, i, magNormalized);
			cm->real[0][i] = cm->real[0][i] * mask;
			cm->imag[0][i] = cm->imag[0][i] * mask;
			cm->real[1][i] = cm->real[1][i] * mask;
			cm->imag[1][i] = cm->imag[1][i] * mask;
		}
	}
	cm->imag[0][0] = cm->imag[1][0] = 0;

	cm->fftBuf[0][0] = cm->real[0][0];
	cm->fftBuf[1][0] = cm->real[1][0];
	for (i = 1; i < cm->halfLen; i++)
	{
		symIdx = cm->fftLen - i;
		unsigned int bitRevFwd = cm->mBitRev[i];
		unsigned int bitRevSym = cm->mBitRev[symIdx];
		cm->fftBuf[0][bitRevFwd] = (cm->real[0][i] + (-cm->imag[0][i])) * cm->shiftCentre[i];
		cm->fftBuf[0][bitRevSym] = (cm->real[0][i] - (-cm->imag[0][i])) * cm->shiftCentre[i];
		cm->fftBuf[1][bitRevFwd] = (cm->real[1][i] + (-cm->imag[1][i])) * cm->shiftCentre[i];
		cm->fftBuf[1][bitRevSym] = (cm->real[1][i] - (-cm->imag[1][i])) * cm->shiftCentre[i];
	}
	cm->fft(cm->fftBuf[0], cm->mSineTab);
	cm->fft(cm->fftBuf[1], cm->mSineTab);
	for (i = 0; i < cm->ovpLen * 2; i++)
	{
		cm->fftBuf[0][i] = cm->fftBuf[0][i + (cm->fftLen / 2 - cm->ovpLen)] * cm->synthesisWnd[i];
		cm->fftBuf[1][i] = cm->fftBuf[1][i + (cm->fftLen / 2 - cm->ovpLen)] * cm->synthesisWnd[i];
	}
	//fwrite(cm->fftBuf, sizeof(float), cm->ovpLen * 2, dbg);
	cm->mOutputBufferCount++;
	float *outBuffer = cm->mOutputBuffer[cm->mOutputBufferCount - 1];
	for (i = 0; i < cm->ovpLen; ++i)
	{
		outBuffer[0] = cm->mOverlapStage2dash[0][0][i] + cm->fftBuf[0][i];
		outBuffer[1] = cm->mOverlapStage2dash[1][0][i] + cm->fftBuf[1][i];
		outBuffer += 2;
		// overlapping
		cm->mOverlapStage2dash[0][0][i] = cm->fftBuf[0][cm->ovpLen + i];
		cm->mOverlapStage2dash[1][0][i] = cm->fftBuf[1][cm->ovpLen + i];
	}
	cm->mInputSamplesNeeded = cm->ovpLen;
}
static inline double fnc1(double x)
{
	return ((606.0 * x * x) / 1087.0 - (3009.0 * x) / 5513.0 + 712.0 / 5411.0) / (x * x - (496.0 * x) / 541.0 + 719.0 / 1034.0);
}
double complexMultiplicationConj(double xReal, double xImag, double yReal, double yImag)
{
	return xReal * yReal - xImag * yImag;
}
void regularSTFT(FFTCompander *cm)
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
	for (i = 0; i < cm->fftLen; i++)
	{
		cm->timeDomainOut[0][i] = cm->timeDomainOut[0][i] * cm->synthesisWnd[i];
		cm->timeDomainOut[1][i] = cm->timeDomainOut[1][i] * cm->synthesisWnd[i];
	}
	cm->mOutputBufferCount++;
	float *outBuffer = cm->mOutputBuffer[cm->mOutputBufferCount - 1];
	unsigned int currentBlockIndex, nextBlockIndex, blockOffset;
	for (i = 0; i < cm->ovpLen; ++i)
	{
		outBuffer[0] = cm->mOverlapStage2dash[0][0][i] + cm->timeDomainOut[0][i];
		outBuffer[1] = cm->mOverlapStage2dash[1][0][i] + cm->timeDomainOut[1][i];
		outBuffer += 2;
		// overlapping
		currentBlockIndex = 0;
		nextBlockIndex = 1;
		blockOffset = cm->ovpLen;
		while (nextBlockIndex < cm->ovpCount - 1) {
			cm->mOverlapStage2dash[0][currentBlockIndex][i] = cm->mOverlapStage2dash[0][nextBlockIndex][i] + cm->timeDomainOut[0][blockOffset + i];
			cm->mOverlapStage2dash[1][currentBlockIndex][i] = cm->mOverlapStage2dash[1][nextBlockIndex][i] + cm->timeDomainOut[1][blockOffset + i];
			currentBlockIndex++;
			nextBlockIndex++;
			blockOffset += cm->ovpLen;
		}
		cm->mOverlapStage2dash[0][currentBlockIndex][i] = cm->timeDomainOut[0][blockOffset + i];
		cm->mOverlapStage2dash[1][currentBlockIndex][i] = cm->timeDomainOut[1][blockOffset + i];
	}
	cm->mInputSamplesNeeded = cm->ovpLen;
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
			cm->process(cm);
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
void LLraisedCosTblFloat(float *dst, int n, int overlapCount)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / n;
	const double scalefac = 1.0 / n;
	float power = 1.0f;
	if (overlapCount == 2)
		power = 0.5f;
	for (int i = 0; i < n; ++i)
		dst[i] = (float)(scalefac * pow(0.5 * (1.0 - cos(twopi_over_n * (i + 0.5))), power));
}
void LLCreatePostWindowFloat(float *dst, int windowSize, int overlapCount)
{
	const float powerIntegrals[8] = { 1.0f, 1.0f / 2.0f, 3.0f / 8.0f, 5.0f / 16.0f, 35.0f / 128.0f,
		63.0f / 256.0f, 231.0f / 1024.0f, 429.0f / 2048.0f };
	int power = 1;
	if (overlapCount == 2)
		power = 0;
	const float scalefac = (float)windowSize * (powerIntegrals[1] / powerIntegrals[power + 1]);
	LLraisedCosTblFloat(dst, windowSize, overlapCount);
	for (int i = 0; i < windowSize; ++i)
		dst[i] *= scalefac;
}
void FFTCompanderInit(FFTCompander *cm, float fs, int tfresolution, unsigned int reqSynthesisWnd, double HFSamplingLimit)
{
	unsigned int i, j, k;
	double oct;
	// FFT method
	if (tfresolution == 0) // STFT
	{
		cm->fftLen = 2048;
		cm->ovpCount = 8;
		cm->process = regularSTFT;
	}
	else if (tfresolution == 1) // CWT
	{
		cm->fftLen = 4096;
		cm->ovpCount = 8;
		oct = 32;
		cm->process = CWTFrameInversion;
	}
	else if (tfresolution == 2) // CWT STFT inverse
	{
		cm->fftLen = 4096;
		cm->ovpCount = 8;
		oct = 32;
		cm->process = CWTFrameSTFTInversion;
	}
	else
	{
		cm->fftLen = 32;
		cm->ovpCount = 1;
	}
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
	cm->ovpLen = cm->fftLen / cm->ovpCount;
	cm->halfLen = (cm->fftLen >> 1) + 1;
	cm->smpShift = (cm->fftLen - (cm->ovpLen << 1));
	const float desiredProcessFreq = 24000.0f;
	unsigned int idx = (unsigned int)(desiredProcessFreq / (fs / (float)cm->fftLen)) + 1UL;
	if (idx > cm->halfLen)
		cm->procUpTo = cm->halfLen;
	else
		cm->procUpTo = idx;
	LLbitReversalTbl(cm->mBitRev, cm->fftLen);
	fhtsinHalfTblFloat(cm->mSineTab, cm->fftLen);
	for (i = 0; i < MAX_OUTPUT_BUFFERS_DRS; i++)
		cm->mOutputBuffer[i] = cm->buffer[i];
	cm->mInputSamplesNeeded = cm->ovpLen;
	cm->mInputPos = 0;
	cm->mOutputBufferCount = 0;
	cm->mOutputReadSampleOffset = 0;
	LLraisedCosTblFloat(cm->analysisWnd, cm->fftLen, cm->ovpCount);
	LLCreatePostWindowFloat(cm->synthesisWnd, cm->fftLen, cm->ovpCount);
	for (i = 0; i < cm->fftLen; ++i)
		cm->synthesisWnd[i] *= 0.5f * (2.0f / (float)cm->ovpCount);
	FFTCompanderSetavgBW(cm, 1.2);
	cm->spectralRate = fs / (float)cm->fftLen * (float)cm->ovpCount;
	for (i = 0; i < HALFWNDLEN_DRS; i++)
		cm->oldBuf[i] = -20.0f;
	unsigned int NFFTDIV2 = cm->fftLen >> 1;
	for (i = 0; i < cm->halfLen; i++)
	{
		if (i % 2 == 0)
			cm->shiftCentre[i] = 1;
		else
			cm->shiftCentre[i] = -1;
	}
	// Initialization
	for (i = 0; i < cm->halfLen; i++)
		cm->corrF[i] = 1.0;
	// Copy overlapping buffer to FFT buffer with fftshift
	unsigned int *idxfftshift = (unsigned int *)malloc(cm->fftLen * sizeof(unsigned int));
	for (i = 0; i < NFFTDIV2; i++)
		idxfftshift[i] = NFFTDIV2 + i;
	for (i = NFFTDIV2; i < cm->fftLen; i++)
		idxfftshift[i] = i - NFFTDIV2;
	for (i = 0; i < cm->fftLen; i++)
		cm->bitrevfftshift[i] = cm->mBitRev[idxfftshift[i]];
	unsigned int zp = 1;
	// number of points of pre andpost padding used to set initial conditions
	cm->prepad = (unsigned int)(cm->fftLen / 102.4);
	cm->pospad = (unsigned int)(cm->fftLen / 102.4) + 2;
	if (tfresolution > 1 && tfresolution < 3)
	{
		double *thetas1 = (double *)malloc((cm->prepad + cm->fftLen + cm->pospad - 1) * sizeof(double));
		for (i = 0; i < cm->halfLen + cm->pospad - 1; i++)
			thetas1[cm->prepad + i] = i;
		thetas1[cm->prepad] = DBL_EPSILON;
		for (i = 0; i < cm->prepad; i++)
			thetas1[cm->prepad - i - 1] = i + 1;
		for (i = 0; i < cm->prepad + cm->halfLen - 1; i++)
			thetas1[cm->prepad + cm->halfLen + i] = thetas1[cm->prepad + cm->halfLen - i - 2];
		double *synWnd = (double *)malloc(cm->fftLen * sizeof(double));
		genWndDBL(synWnd, cm->fftLen, "hann");
		double *analysisWnd = (double *)malloc(cm->fftLen * sizeof(double));
		double *b = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		double *c1 = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		double *c2 = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		for (i = 0; i < cm->fftLen; i++)
		{
			analysisWnd[i] = pow(synWnd[i], 1.0 / reqSynthesisWnd);
			cm->analysisWnd[i] = (float)analysisWnd[i];
		}
		double *chopedWnd1 = analysisWnd + cm->halfLen - 1;
		double *chopedWnd2 = synWnd + cm->halfLen - 1;
		unsigned int div = (unsigned int)round(cm->ovpLen / (1 + HFSamplingLimit));
		unsigned int halfWndLen = cm->halfLen - 1;
		double *digw = (double *)malloc(halfWndLen * sizeof(double));
		linspace(digw, 0, M_PI - M_PI / halfWndLen, halfWndLen);
		digw[halfWndLen - 1] = M_PI - M_PI / halfWndLen;
		double sRe = cos(digw[div - 1]);
		double sIm = sin(digw[div - 1]);
		double *hHopPt = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		double tmpRe, tmpIm;
		complexMultiplication(sRe, sIm, sRe, sIm, &tmpRe, &tmpIm);
		double wndPwr = pow(chopedWnd2[div - 1], reqSynthesisWnd);
		double top = -DBL_MIN, down = DBL_MAX;
		for (i = 0; i < (cm->prepad + cm->halfLen + cm->pospad - 1); i++)
		{
			double sigmas = (thetas1[i] / cm->fftLen) / oct / M_PI * cm->fftLen;
			double mp = exp(-137 / (100 * sigmas));
			double a1 = 2.0 * cos(fnc1(sigmas) / sigmas) * mp;
			double a[3] = { 1.0, -a1, mp * mp };
			double b = a[0] + a[1] + a[2];
			c1[i] = 2.0 - a1;
			c2[i] = b / c1[i];
			double bDeflatedRe = tmpRe * b;
			double bDeflatedIm = tmpIm * b;
			double aDeflatedRe = tmpRe + sRe * a[1] + a[2];
			double aDeflatedIm = tmpIm + sIm * a[1];
			double hHopPtCplxRe, hHopPtCplxIm;
			//printf("%d %1.14lf %1.14lf %1.14lf %1.14lf\n", i + 1, bDeflatedRe, bDeflatedIm, aDeflatedRe, aDeflatedIm);
			cdivid(bDeflatedRe, bDeflatedIm, aDeflatedRe, aDeflatedIm, &hHopPtCplxRe, &hHopPtCplxIm);
			hHopPt[i] = complexMultiplicationConj(hHopPtCplxRe, hHopPtCplxIm, hHopPtCplxRe, -hHopPtCplxIm) * chopedWnd1[div - 1] * wndPwr;
			if (hHopPt[i] > top)
				top = hHopPt[i];
			if (hHopPt[i] < down)
				down = hHopPt[i];
		}
		double thres = (top + down) / 2.0;
		double *shiftedhHopPt = hHopPt + cm->prepad;
		char isEmpty = 1;
		for (i = 0; i < cm->halfLen; i++)
		{
			if (shiftedhHopPt[i] <= thres)
			{
				isEmpty = 0;
				break;
			}
		}
		unsigned int firstUndersampling = cm->prepad + min(i + 1, (unsigned int)(cm->fftLen / 4 - 1) - cm->prepad) - 1;
		if (!isEmpty)
		{
			double thetaclipping = thetas1[firstUndersampling - 1];
			unsigned int len = cm->halfLen - firstUndersampling + 1;
			for (i = 0; i < len; i++)
				thetas1[i + firstUndersampling - 1] = thetaclipping;
			for (i = 0; i < cm->prepad; i++)
				thetas1[i] = thetas1[cm->prepad * 2 - i];
			len = cm->halfLen - (cm->halfLen - cm->prepad - cm->pospad + 1);
			for (i = 0; i < len; i++)
				thetas1[cm->halfLen + i] = thetas1[cm->halfLen - i - 2];
			// Eliminate oscillation around corner
			double time = 0.026 * cm->fftLen / zp; // More elements in array less smoothing is needed
			double alpha = 1 / (1 + time);
			double alph = tan((M_PI * alpha) / 2.0);
			double bCoef = alph / (1.0 + alph);
			double iir_b[2] = { bCoef, bCoef };
			double iir_a = -(1.0 - alph) / (1.0 + alph);
			double zi = (iir_b[1] - iir_b[0] * iir_a) / (1.0 + iir_a);
			double *ytemp = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1 + 2 * 3) * sizeof(double));
			ytemp[0] = 2.0 * thetas1[0] - thetas1[3];
			ytemp[1] = 2.0 * thetas1[0] - thetas1[2];
			ytemp[2] = 2.0 * thetas1[0] - thetas1[1];
			memcpy(ytemp + 3, thetas1, (cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
			ytemp[3 + (cm->prepad + cm->halfLen + cm->pospad - 1) + 0] = 2.0 * thetas1[cm->prepad + cm->halfLen + cm->pospad - 2] - thetas1[cm->prepad + cm->halfLen + cm->pospad - 3];
			ytemp[3 + (cm->prepad + cm->halfLen + cm->pospad - 1) + 1] = 2.0 * thetas1[cm->prepad + cm->halfLen + cm->pospad - 2] - thetas1[cm->prepad + cm->halfLen + cm->pospad - 4];
			ytemp[3 + (cm->prepad + cm->halfLen + cm->pospad - 1) + 2] = 2.0 * thetas1[cm->prepad + cm->halfLen + cm->pospad - 2] - thetas1[cm->prepad + cm->halfLen + cm->pospad - 5];
			double zLPF = zi * ytemp[0];
			for (i = 0; i < (cm->prepad + cm->halfLen + cm->pospad - 1 + 2 * 3); i++)
			{
				double out = ytemp[i] * bCoef + zLPF;
				zLPF = ytemp[i] * bCoef - iir_a * out;
				ytemp[i] = out;
			}
			int left = 0;
			int right = (cm->prepad + cm->halfLen + cm->pospad - 1 + 2 * 3 - 1);
			while (left < right)
			{
				double tmp = ytemp[left];
				ytemp[left++] = ytemp[right];
				ytemp[right--] = tmp;
			}
			zLPF = zi * ytemp[0];
			for (i = 0; i < (cm->prepad + cm->halfLen + cm->pospad - 1 + 2 * 3); i++)
			{
				double out = ytemp[i] * bCoef + zLPF;
				zLPF = ytemp[i] * bCoef - iir_a * out;
				ytemp[i] = out;
			}
			left = 0;
			right = (cm->prepad + cm->halfLen + cm->pospad - 1 + 2 * 3 - 1);
			while (left < right)
			{
				double tmp = ytemp[left];
				ytemp[left++] = ytemp[right];
				ytemp[right--] = tmp;
			}
			memcpy(thetas1, ytemp + 3, (cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
			free(ytemp);
			for (i = 0; i < (cm->prepad + cm->halfLen + cm->pospad - 1); i++)
			{
				double sigmas = (thetas1[i] / cm->fftLen) / oct / M_PI * cm->fftLen;
				double mp = exp(-137 / (100 * sigmas));
				double a1 = 2.0 * cos(fnc1(sigmas) / sigmas) * mp;
				double a[3] = { 1.0, -a1, mp * mp };
				b[i] = a[0] + a[1] + a[2];
				c1[i] = 2.0 - a1;
				c2[i] = b[i] / c1[i];
			}
			//for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
			//	printf("%d %1.14lf %1.14lf %1.14lf\n", i + 1, b[i], c1[i], c2[i]);
		}
		double meaTheta = 0.0;
		for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
			meaTheta += thetas1[i];
		meaTheta /= (cm->prepad + cm->halfLen + cm->pospad - 1);
		char flat = 0;
		for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
		{
			if (fabs(thetas1[i] - meaTheta) < DBL_EPSILON)
			{
				flat = 1;
				break;
			}
		}
		double *mSineTab = (double *)malloc(cm->fftLen * sizeof(double));
		LLsinHalfTbl(mSineTab, cm->fftLen);
		unsigned int xlen = cm->fftLen + ((cm->ovpCount - 1) - 1) * cm->ovpLen;
		double *systemImpulse = (double *)malloc(xlen * sizeof(double));
		memset(systemImpulse, 0, xlen * sizeof(double));
		double *tmpreal = (double *)malloc(cm->halfLen * sizeof(double));
		double *tmpimag = (double *)malloc(cm->halfLen * sizeof(double));
		double *tmpspecHannReal = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		double *tmpspecHannImag = (double *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(double));
		cplxDouble *tmCplx = (cplxDouble *)malloc((cm->prepad + cm->halfLen + cm->pospad - 1) * sizeof(cplxDouble));
		double *fftBuf = (double *)malloc(cm->fftLen * sizeof(double));
		for (j = 0; j < cm->ovpCount - 1; j++)
		{
			unsigned int stepSize1 = (cm->fftLen - cm->ovpLen * (j + 1)) % cm->fftLen;
			unsigned int stepSize = (cm->fftLen - cm->ovpLen * (j + 1) - cm->fftLen / 2 - 1) % cm->fftLen;
			double cons1 = stepSize * 2 * M_PI / (double)cm->fftLen;
			double anawnd = cm->analysisWnd[stepSize1 - 1];
			for (i = 0; i < cm->halfLen; i++)
			{
				double idx = -cons1 * i;
				tmpspecHannReal[i + cm->prepad] = anawnd * cos(idx);
				tmpspecHannImag[i + cm->prepad] = anawnd * sin(idx);
			}
			for (i = 0; i < cm->prepad; ++i)
			{
				tmpspecHannReal[cm->prepad - i - 1] = tmpspecHannReal[cm->prepad + i + 1];
				tmpspecHannImag[cm->prepad - i - 1] = -tmpspecHannImag[cm->prepad + i + 1];
			}
			for (i = 0; i < cm->pospad - 1; ++i)
			{
				tmpspecHannReal[cm->prepad + cm->halfLen + cm->pospad - i - 2] = tmpspecHannReal[cm->prepad + cm->halfLen - cm->pospad + i];
				tmpspecHannImag[cm->prepad + cm->halfLen + cm->pospad - i - 2] = -tmpspecHannImag[cm->prepad + cm->halfLen - cm->pospad + i];
			}
			// LTV Gaussian
			cplxDouble z1, z2, o, st;
			z1.real = z1.imag = 0;
			z2.real = z2.imag = 0;
			for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
			{
				o.real = tmpspecHannReal[i] - z1.real - z2.real;
				o.imag = tmpspecHannImag[i] - z1.imag - z2.imag;
				st.real = c2[i] * z1.real;
				st.imag = c2[i] * z1.imag;
				tmCplx[i].real = b[i] * o.real + 2 * st.real + z2.real;
				tmCplx[i].imag = b[i] * o.imag + 2 * st.imag + z2.imag;
				z2.real = z2.real + st.real;
				z2.imag = z2.imag + st.imag;
				z1.real = z1.real + c1[i] * o.real;
				z1.imag = z1.imag + c1[i] * o.imag;
			}
			z1.real = z1.imag = 0;
			z2.real = z2.imag = 0;
			for (i = cm->prepad + cm->halfLen + cm->pospad - 1; i-- > 0; )
			{
				o.real = tmCplx[i].real - z1.real - z2.real;
				o.imag = tmCplx[i].imag - z1.imag - z2.imag;
				st.real = c2[i] * z1.real;
				st.imag = c2[i] * z1.imag;
				tmpspecHannReal[i] = b[i] * o.real + 2 * st.real + z2.real;
				tmpspecHannImag[i] = b[i] * o.imag + 2 * st.imag + z2.imag;
				z2.real = z2.real + st.real;
				z2.imag = z2.imag + st.imag;
				z1.real = z1.real + c1[i] * o.real;
				z1.imag = z1.imag + c1[i] * o.imag;
			}
			//for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; ++i)
			//	printf("%d %1.14lf %1.14lf\n", i + 1, tmpspecHannReal[i], tmpspecHannImag[i]);
			for (i = cm->prepad; i < cm->prepad + cm->halfLen; ++i)
			{
				tmpspecHannReal[i - cm->prepad] = tmpspecHannReal[i] * cm->corrF[i - cm->prepad];
				tmpspecHannImag[i - cm->prepad] = tmpspecHannImag[i] * cm->corrF[i - cm->prepad];
			}
			tmpspecHannImag[0] = tmpspecHannImag[cm->halfLen - 1] = 0;
			double *reTar = tmpreal;
			double *imTar = tmpimag;
			double *reSrc = tmpspecHannReal;
			double *imSrc = tmpspecHannImag;
			// Inverse transform
			for (k = 0; k < reqSynthesisWnd; k++)
			{
				reTar[0] = (reSrc[0] * 2.0 + reSrc[1] + reSrc[1]) / 4.0; // DC
				imTar[0] = 0.0; // DC
				for (i = 1; i < cm->halfLen - 1; i++)
				{
					reTar[i] = (reSrc[i] * 2.0 + reSrc[i - 1] + reSrc[i + 1]) / 4.0;
					imTar[i] = (imSrc[i] * 2.0 + imSrc[i - 1] + imSrc[i + 1]) / 4.0;
				}
				reTar[cm->halfLen - 1] = (reSrc[cm->halfLen - 1] * 2.0 + reSrc[cm->halfLen - 2] + reSrc[cm->halfLen - 2]) / 4.0; // Nyquist
				imTar[cm->halfLen - 1] = 0.0;
				double *retmp = reSrc;
				double *imtmp = imSrc;
				reSrc = reTar;
				imSrc = imTar;
				reTar = retmp;
				imTar = imtmp;
			}
			fftBuf[0] = reSrc[0];
			for (i = 1; i < cm->halfLen; i++)
			{
				unsigned int symIdx = cm->fftLen - i;
				unsigned int bitRevFwd = cm->mBitRev[i];
				unsigned int bitRevSym = cm->mBitRev[symIdx];
				fftBuf[bitRevFwd] = reSrc[i] + (-imSrc[i]);
				fftBuf[bitRevSym] = reSrc[i] - (-imSrc[i]);
			}
			//for (i = 0; i < cm->halfLen; ++i)
			//	printf("%d %1.14lf %1.14lf\n", i + 1, tmpreal[i], tmpimag[i]);
			discreteHartleyTransform(fftBuf, cm->fftLen, mSineTab);
			double *ptr = systemImpulse + j * cm->ovpLen;
			for (i = 0; i < cm->fftLen; ++i)
				ptr[i] += fftBuf[idxfftshift[i]] / cm->fftLen;
		}
		double *truncatedSystemImpulse = (double *)malloc((cm->fftLen + 1) * sizeof(double));
		if (cm->ovpLen != NFFTDIV2)
		{
			xlen = (cm->fftLen - cm->ovpLen) - ((cm->fftLen - cm->ovpLen) - (cm->fftLen / 2)) + 1;
			memcpy(truncatedSystemImpulse, systemImpulse + (cm->fftLen - cm->ovpLen) - (cm->fftLen / 2) - 1, xlen * sizeof(double));
		}
		else
		{
			xlen = cm->fftLen - cm->ovpLen;
			truncatedSystemImpulse[0] = 0;
			memcpy(truncatedSystemImpulse + 1, systemImpulse, xlen * sizeof(double));
		}
		for (i = 0; i < NFFTDIV2; i++)
			truncatedSystemImpulse[cm->halfLen + i] = truncatedSystemImpulse[cm->halfLen - 2 - i];
		double *truncatedSystemImpulseptr = truncatedSystemImpulse + 1;
		for (i = 0; i < cm->fftLen; i++)
			fftBuf[cm->mBitRev[i]] = truncatedSystemImpulseptr[i];
		discreteHartleyTransform(fftBuf, cm->fftLen, mSineTab);
		double *impulseSpectrum = (double *)malloc(cm->halfLen * sizeof(double));
		impulseSpectrum[0] = fftBuf[0];
		unsigned int symIdx;
		double mm = fftBuf[0];
		for (i = 1; i < cm->halfLen; i++)
		{
			symIdx = cm->fftLen - i;
			impulseSpectrum[i] = hypot(fftBuf[i] + fftBuf[symIdx], -(fftBuf[i] - fftBuf[symIdx])) * 0.5;
			mm += impulseSpectrum[i];
		}
		if (flat)
		{
			mm = 1.0 / (mm / cm->halfLen);
			for (i = 0; i < cm->halfLen; i++)
				cm->corrF[i] = (float)mm;
		}
		else
		{
			for (i = 0; i < cm->halfLen; i++)
				cm->corrF[i] = (float)(1.0 / impulseSpectrum[i]);
		}
		double *synth_win = (double *)malloc(cm->fftLen * sizeof(double));
		memset(synth_win, 0, cm->fftLen * sizeof(double));
		genWndDBL(synth_win + cm->fftLen / 2 - cm->ovpLen, cm->ovpLen * 2, "hann");
		double *tmpWnd = (double *)malloc(cm->fftLen * sizeof(double));
		for (i = 0; i < cm->fftLen; i++)
			tmpWnd[i] = synth_win[i] * cm->analysisWnd[i];
		xlen = cm->fftLen + (cm->ovpCount * 2 - 1) * cm->ovpLen;
		double *correctionWndHF = (double *)malloc(xlen * sizeof(double));
		memset(correctionWndHF, 0, xlen * sizeof(double));
		for (j = 0; j < cm->ovpCount * 2; j++)
		{
			double *ptr = correctionWndHF + j * cm->ovpLen;
			for (i = 0; i < cm->fftLen; ++i)
				ptr[i] += tmpWnd[i];
		}
		double *wndValidPart1 = synth_win + (cm->fftLen / 2 - cm->ovpLen);
		double *wndValidPart2 = correctionWndHF + (cm->fftLen - cm->ovpLen) + (cm->fftLen / 2 - cm->ovpLen);
		for (j = 0; j < cm->ovpLen * 2; j++)
			cm->synthesisWnd[j] = (float)((wndValidPart1[j] / wndValidPart2[j]) / (cm->fftLen * 2.0));
		/*FILE *fpp = fopen("dd.dat", "wb");
		fwrite(cm->synthesisWnd, sizeof(float), cm->ovpLen * 2, fpp);
		fclose(fpp);*/
		/*FILE *fpp = fopen("dd.dat", "wb");
		fwrite(cm->corrF, sizeof(float), cm->halfLen, fpp);
		fclose(fpp);*/
		for (i = 0; i < cm->prepad + cm->halfLen + cm->pospad - 1; i++)
		{
			cm->gauss_b[i] = (float)b[i];
			cm->gauss_c1[i] = (float)c1[i];
			cm->gauss_c2[i] = (float)c2[i];
		}
		free(thetas1);
		free(synWnd);
		free(analysisWnd);
		free(b);
		free(c1);
		free(c2);
		free(digw);
		free(hHopPt);
		free(mSineTab);
		free(systemImpulse);
		free(tmpreal);
		free(tmpimag);
		free(tmpspecHannReal);
		free(tmpspecHannImag);
		free(tmCplx);
		free(fftBuf);
		free(truncatedSystemImpulse);
		free(impulseSpectrum);
		free(synth_win);
		free(tmpWnd);
		free(correctionWndHF);
	}
	else if (tfresolution == 1)
	{
		memcpy(cm->gauss_b, prec_b, sizeof(prec_b));
		memcpy(cm->gauss_c1, prec_c1, sizeof(prec_c1));
		memcpy(cm->gauss_c2, prec_c2, sizeof(prec_c2));
		memcpy(cm->corrF, prec_corrF, sizeof(prec_corrF));
		cm->scalarGain = (float)(1.0 / cm->fftLen / 2.0);
	}
	free(idxfftshift);
	if (tfresolution == 3)
	{
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
}
void CompressorDestructor(JamesDSPLib *jdsp)
{
	freeIerper(&jdsp->comp.pch);
}
void CompressorEnable(JamesDSPLib *jdsp, char enable)
{
	if (jdsp->compForceRefresh)
	{
		CompressorSetParam(jdsp, jdsp->comp.fgt_facT, jdsp->comp.granularity, jdsp->comp.tfresolution, 1);
		CompressorSetGain(jdsp, 0, 0, 0);
	}
	if (enable)
		jdsp->compEnabled = 1;
}
void CompressorDisable(JamesDSPLib *jdsp)
{
	jdsp->compEnabled = 0;
}
void CompressorSetParam(JamesDSPLib *jdsp, float fgt_facT, int granularity, int tfresolution, char forceRefresh)
{
	FFTCompander *cm = (FFTCompander *)(&jdsp->comp);
	if ((fgt_facT != cm->fgt_facT || tfresolution != cm->tfresolution || granularity != cm->granularity) || forceRefresh)
	{
		FFTCompanderInit(&jdsp->comp, jdsp->fs, tfresolution, 3, 1.0);
		cm->fgt_facT = fgt_facT;
		cm->tfresolution = tfresolution;
		cm->granularity = granularity;
		if (cm->tfresolution < 3)
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
				avgBW = 1.08;
			FFTCompanderSetavgBW(cm, avgBW);
		}
		else if (cm->tfresolution == 3)
		{
			cm->fgt_fac = (float)(1.0 - exp(-1.0 / (cm->fgt_facT * jdsp->fs)));
		}
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
	pchip(&cm->pch, cm->freq2, cm->gains2, NUMPTS_DRS + 2, 1, 1);
	unsigned int specLen = *((unsigned int *)(cm->octaveSmooth));
	float reciprocal = *((float *)(cm->octaveSmooth + sizeof(unsigned int)));
	unsigned int lpLen = *((unsigned int *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float)));
	float *lv1 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float));
	float *lv2 = (float *)(cm->octaveSmooth + sizeof(unsigned int) + sizeof(float) + sizeof(unsigned int) + (lpLen << 1) * sizeof(unsigned int) + lpLen * sizeof(float) + (lpLen + 3) * sizeof(float));
	if (cm->tfresolution < 3)
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
	if (cm->tfresolution < 3)
	{
		unsigned int offset = 0;
		while (offset < n)
		{
			const unsigned int processing = min(n - offset, cm->ovpLen);
			FFTCompanderProcessSamples(cm, jdsp->tmpBuffer[0] + offset, jdsp->tmpBuffer[1] + offset, processing, jdsp->tmpBuffer[0] + offset, jdsp->tmpBuffer[1] + offset);
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