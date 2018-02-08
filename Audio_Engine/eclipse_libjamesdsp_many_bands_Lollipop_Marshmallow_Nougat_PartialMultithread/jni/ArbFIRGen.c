#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "ArbFIRGen.h"
#define PI 3.141592653589793
#define PI2 6.283185307179586
int get_float(char *val, float *F)
{
    char *eptr;
    errno = 0;
    float f = strtof(val, &eptr);
    if (eptr != val && errno != ERANGE)
    {
        *F = f;
        return 1;
    }
    return 0;
}
int cmpfuncD(const void *a, const void *b)
{
    if (*(float*)a - *(float*)b < 0)
        return -1;
    if (*(float*)a - *(float*)b > 0)
        return 1;
    return 0;
}
int lower_bound(EqNode **array, int size, float key)
{
    int first = 0, half, len, middle;
    len = size;
    while (len > 0)
    {
        half = len >> 1;
        middle = first + half;
        if (array[middle]->freq < key)
        {
            first = middle + 1;
            len = len - half - 1;
        }
        else
            len = half;
    }
    return first;
}
#define EPS	1E-16
double bessi0(double x)
{
	double xh, sum, pow, ds;
	int k;
	xh = 0.5 * x;
	sum = 1.0;
	pow = 1.0;
	k = 0;
	ds = 1.0;
	while (ds > sum * EPS)
	{
		++k;
		pow = pow * (xh / k);
		ds = pow * pow;
		sum = sum + ds;
	}
	return sum;
}
#undef EPS
double getKaiser(double x, double alpha)
{
	static double alpha_prev = 0.0;
	static double Ia = 1.0;
	double beta, win;
	if (alpha != alpha_prev)
	{
		Ia = bessi0(alpha);
		alpha_prev = alpha;
	}
	if (x < -1.0 || x > 1.0)
		win = 0.0;
	else
	{
		beta = alpha * sqrt(1.0 - x * x);
		win = bessi0(beta) / Ia;
	}
	return win;
}
void winKaiser(float *win, int N, double alpha)
{
	int i, k, minus1 = N - 1;
	for (i = 0, k = minus1; i <= k; ++i, --k)
	{
		win[i] = (float)getKaiser((double)(2 * i - minus1) / minus1, alpha);
		win[k] = win[i];
	}
}
void winKaiserHalf(float *win, int N, double alpha)
{
	int i, mulN = (N << 1) - 1;
	for (i = 0; i < N; i++)
		win[N - 1 - i] = (float)getKaiser((double)(2 * i - mulN) / mulN, alpha);
}
//   FIR filter design using the window method - arbitrary filter shape.
//   fir2(int *nn, double *ff, double *aa, int ffsz) designs an N'th order FIR digital filter with the
//   frequency response specified by vectors F and M (size NF) and returns the
//   filter coefficients in length N+1 vector B.  Vectors F and M specify
//   the frequency and magnitude breakpoints for the filter such that
//   PLOT(F,M) would show a plot of the desired frequency response.
//   The frequencies in F must be between 0.0 < F < 1.0, with 1.0
//   corresponding to half the sample rate. They must be in increasing
//   order and start with 0.0 and end with 1.0.
//
//   The filter B is real, and has linear phase, i.e., even symmetric
//   coefficients obeying B(k) =  B(N+2-k), k = 1,2,...,N+1.
//   float fs = 48000.0f;
//   float fc = 120.0f;
//   float gain = 5.0f;
//   float freq[4] = { 0 / fs, (fc * 2.0) / fs, (fc * 2.0 + 80.0) / fs , 1.0 };
//   float amplitude[4] = { gain, gain, 0, 0 };
//   float *freqSamplImp;
//   freqSamplImp = fir2(&size, freq, amplitude, 4);
//   .......
//   free(freqSamplImp);
//   FIR2 uses a Hamming window.
inline void diff(float *y, float *f, int sz)
{
	--sz;
	for (int i = 0; i < sz; i++)
		f[i] = y[i + 1] - y[i];
}
inline int isneg(float *y, int sz)
{
	for (int i = 0; i < sz; i++)
		if (y[i] < 0) return 1;
	return 0;
}
float* fir2(int *nn, float *ff, float *aa, int ffsz)
{
	int npt, lap, npt2;
	// Convert gain to linear scale
	for (lap = 0; lap < ffsz; lap++)
		aa[lap] = powf(10.0f, aa[lap] / 20.0f);
	if (!(*nn % 2))
		*nn += 1;
	if (*nn < 1024)
		npt = 512;
	else
		npt = (int)pow(2.0, ceil(log((double)*nn) / log(2.0)));
	lap = (int)(npt / 25);
	if (fabsf(ff[0]) > 0 || fabsf(ff[ffsz - 1] - 1) > 1)
	{
//		printf("The first frequency must be 0 and the last 1");
		return 0;
	}
	// Interpolate breakpoints onto large grid
	int nint = ffsz - 1;
	float *df = (float*)malloc(sizeof(float)*(ffsz - 1));
	diff(ff, df, ffsz);
	if (isneg(df, ffsz - 1))
	{
//		printf("Frequencies must be non-decreasing");
		return 0;
	}
	npt = npt + 1; // Length of [dc 1 2 ... nyquist] frequencies.
	npt2 = npt * 2;
	int nb = 0;
	int ne = 0;
	float inc;
	float *H = (float*)malloc(sizeof(float)*npt);
	H[0] = aa[0];
	int i;
	for (i = 0; i < nint; i++)
	{
		if (df[i] == 0)
		{
			nb = nb - lap / 2;
			ne = nb + lap;
		}
		else
			ne = (int)(ff[i + 1] * npt) - 1;
		if (nb < 0 || ne > npt)
		{
//			printf("Too abrupt an amplitude change near end of frequency interval");
			return 0;
		}
		int j;
		for (j = nb; j <= ne; j++)
		{
			if (nb == ne)
				inc = 0;
			else
				inc = (float)(j - nb) / (float)(ne - nb);
			H[j] = inc*aa[i + 1] + (1.0f - inc)*aa[i];
		}
		nb = ne + 1;
	}
	// Fourier time-shift.
	float dt = 0.5f * (float)(*nn - 1);
	fftwf_complex *Hz2 = (fftwf_complex*)fftwf_malloc(npt * sizeof(fftwf_complex));
	fftwf_complex *Hz = (fftwf_complex*)fftwf_malloc(npt2 * sizeof(fftwf_complex));
	for (i = 0; i < npt; i++)
	{
		float rad = -dt * PI * (float)i / ((float)(npt - 1));
		float Hz1Real = H[i] * cosf(rad);
		float Hz1Imag = H[i] * sinf(rad);
		Hz2[npt - 1 - i][1] = -1.0f * Hz1Imag;
		Hz2[npt - 1 - i][0] = Hz1Real;
		Hz[i][0] = Hz1Real;
		Hz[i][1] = Hz1Imag;
	}
	for (i = npt; i < npt2; i++)
	{
		Hz[i][0] = Hz2[i - npt][0];
		Hz[i][1] = Hz2[i - npt][1];
	}
	int nfft = npt2 - 2;
	float *fo = (float*)fftwf_malloc(sizeof(float) * npt2);
	fftwf_plan plan = fftwf_plan_dft_c2r_1d(nfft, Hz, fo, FFTW_ESTIMATE);
	fftwf_execute(plan);
	inc = (float)(*nn - 1);
	float kfft = 1. / nfft;
	float *retArray = (float*)malloc(*nn * sizeof(float));
	for (i = 0; i < *nn; i++)
		retArray[i] = (float)(0.54 - (0.46*cos(PI2*(double)i / (double)inc))) * fo[i] * kfft;
	fftwf_destroy_plan(plan);
	fftwf_free(Hz);
	fftwf_free(fo);
	fftwf_free(Hz2);
	free(df);
	free(H);
	return retArray;
}
/////////////////////////////////////////////////////////////////////////////
// Log grid interpolated arbitrary response FIR filter design
/////////////////////////////////////////////////////////////////////////////
void minimumPhaseSpectrum(fftwf_complex* timeData, fftwf_complex* freqData, fftwf_plan planForward, fftwf_plan planReverse, unsigned int filterLength)
{
    unsigned int i, fLMul2 = filterLength << 1;
    float threshold = powf(10.0f, -100.0f / 20.0f);
    float logThreshold = logf(threshold);
    for (i = 0; i < fLMul2; i++)
    {
        if (freqData[i][0] < threshold)
            freqData[i][0] = logThreshold;
        else
            freqData[i][0] = logf(freqData[i][0]);
        freqData[i][1] = 0;
    }
    fftwf_execute(planReverse);
    for (i = 0; i < fLMul2; i++)
    {
        timeData[i][0] /= fLMul2;
        timeData[i][1] /= fLMul2;
    }
    for (i = 1; i < filterLength; i++)
    {
        timeData[i][0] += timeData[fLMul2 - i][0];
        timeData[i][1] -= timeData[fLMul2 - i][1];
        timeData[fLMul2 - i][0] = 0;
        timeData[fLMul2 - i][1] = 0;
    }
    timeData[filterLength][1] *= -1.0;
    fftwf_execute(planForward);
    for (i = 0; i < fLMul2; i++)
    {
        float eR = expf(freqData[i][0]);
        freqData[i][0] = eR * cosf(freqData[i][1]);
        freqData[i][1] = eR * sinf(freqData[i][1]);
    }
}
inline float gainAtLogGrid(ArbitraryEq *gains, float freq)
{
    float dbGain = 0.0f, logLeft, logRightMinusLeft;
    if (!gains->nodesCount)
         return dbGain;
     if (gains->nodesCount == 1)
         return gains->nodes[0]->gain;
     else
     {
         if (freq > gains->nodes[gains->nodesCount - 1]->freq)
             return gains->nodes[gains->nodesCount - 1]->gain;
         else
         {
             int pos = lower_bound(gains->nodes, gains->nodesCount, freq);
             if (pos < 1)
                 return gains->nodes[0]->gain;
 			float tmpfreq = gains->nodes[pos - 1]->freq;
 			if (tmpfreq < 2.0f)
 				logLeft = tmpfreq;
 			else
 				logLeft = logf(tmpfreq);
 			tmpfreq = gains->nodes[pos]->freq;
 			if (tmpfreq < 2.0f)
 				logRightMinusLeft = tmpfreq - logLeft;
 			else
 				logRightMinusLeft = logf(tmpfreq) - logLeft;
 			float t;
 			if (freq < 2.0f)
 				t = (freq - logLeft) / logRightMinusLeft;
 			else
 				t = (logf(freq) - logLeft) / logRightMinusLeft;
 			if (gains->nodes[pos - 1]->gain == gains->nodes[pos]->gain)
 				dbGain = gains->nodes[pos]->gain;
 			else
 				dbGain = gains->nodes[pos - 1]->gain + t * (gains->nodes[pos]->gain - gains->nodes[pos - 1]->gain);
         }
     }
    return dbGain;
}
float *ArbitraryEqMinimumPhase(ArbitraryEq *gains, float fs)
{
    unsigned int fLMul2 = gains->fLMul2;
    unsigned int flMul2Minus1 = fLMul2 - 1;
    fftwf_complex* timeData = gains->timeData;
    fftwf_complex* freqData = gains->freqData;
    fftwf_plan planForward = gains->planForward;
    fftwf_plan planReverse = gains->planReverse;
    // Log grid interpolation
    unsigned int i;
    for (i = 0; i < gains->filterLength; i++)
    {
        float freq = (float)i * fs / (float)fLMul2;
        float dbGain = gainAtLogGrid(gains, freq);
        float gain = powf(10.0f, dbGain / 20.0f);
        freqData[i][0] = gain;
        freqData[i][1] = 0;
        freqData[flMul2Minus1 - i][0] = gain;
        freqData[flMul2Minus1 - i][1] = 0;
    }
    minimumPhaseSpectrum(timeData, freqData, planForward, planReverse, gains->filterLength);
    fftwf_execute(planReverse);
	float factor, *finalImpulse = gains->impulseResponse;
    for (i = 0; i < gains->filterLength; i++)
    {
        factor = (float)(0.5 * (1.0 + cos(PI2 * (double)i / (double)fLMul2)));
		finalImpulse[i] = factor / (float)fLMul2 * timeData[i][0];
    }
    return finalImpulse;
}
float *ArbitraryEqLinearPhase(ArbitraryEq *gains, float fs)
{
    unsigned int fLMul2 = gains->fLMul2;
    unsigned int flMul2Minus1 = fLMul2 - 1;
    fftwf_complex* timeData = gains->timeData;
    fftwf_complex* freqData = gains->freqData;
    fftwf_plan planReverse = gains->planReverse;
    // Log grid interpolation
    unsigned int i;
    for (i = 0; i < gains->filterLength; i++)
    {
        float freq = (float)i * fs / (float)fLMul2;
        float dbGain = gainAtLogGrid(gains, freq);
        float gain = powf(10.0f, dbGain / 20.0f);
        freqData[i][0] = gain;
        freqData[i][1] = 0;
        freqData[flMul2Minus1 - i][0] = gain;
        freqData[flMul2Minus1 - i][1] = 0;
    }
    fftwf_execute(planReverse);
    float *finalImpulse = gains->impulseResponse;
    flMul2Minus1 = gains->filterLength - 1;
	for (i = 0; i < gains->filterLength; i++)
	{
		float timeIntermediate = timeData[i][0] * 0.5f * (float)((1.0 + cos(PI2 * (double)i / (double)fLMul2)) / (double)fLMul2);
		finalImpulse[flMul2Minus1 - i] = finalImpulse[flMul2Minus1 + i] = timeIntermediate;
	}
    return finalImpulse;
}
void InitArbitraryEq(ArbitraryEq* eqgain, int *filterLength, int isLinearPhase)
{
    eqgain->fLMul2 = *filterLength << 1;
    eqgain->timeData = fftwf_alloc_complex(eqgain->fLMul2);
    eqgain->freqData = fftwf_alloc_complex(eqgain->fLMul2);
    eqgain->filterLength = *filterLength;
    eqgain->isLinearPhase = isLinearPhase;
	eqgain->nodes = 0;
	eqgain->nodesCount = 0;
    if (!isLinearPhase)
    {
        eqgain->planForward = fftwf_plan_dft_1d(eqgain->fLMul2, eqgain->timeData, eqgain->freqData, FFTW_FORWARD, FFTW_ESTIMATE);
        eqgain->impulseResponse = (float*)malloc(*filterLength * sizeof(float));
        eqgain->GetFilter = &ArbitraryEqMinimumPhase;
    }
    else
    {
        eqgain->impulseResponse = (float*)malloc(eqgain->fLMul2 * sizeof(float));
        eqgain->GetFilter = &ArbitraryEqLinearPhase;
        *filterLength = eqgain->fLMul2;
    }
    eqgain->planReverse = fftwf_plan_dft_1d(eqgain->fLMul2, eqgain->freqData, eqgain->timeData, FFTW_BACKWARD, FFTW_ESTIMATE);
}
void EqNodesFree(ArbitraryEq *eqgain)
{
    for (unsigned int i = 0; i < eqgain->nodesCount; i++)
        free(eqgain->nodes[i]);
    free(eqgain->nodes);
    eqgain->nodes = 0;
    eqgain->nodesCount = 0;
}
void ArbitraryEqFree(ArbitraryEq *eqgain)
{
    EqNodesFree(eqgain);
    if (!eqgain->isLinearPhase)
        fftwf_destroy_plan(eqgain->planForward);
    fftwf_free(eqgain->freqData);
    fftwf_destroy_plan(eqgain->planReverse);
    free(eqgain->impulseResponse);
}
void NodesSorter(ArbitraryEq *eqgain)
{
    unsigned int i, numOfNodes = eqgain->nodesCount;
    float *freqArray = (float*)malloc(numOfNodes * sizeof(float));
    for (i = 0; i < numOfNodes; i++)
        freqArray[i] = eqgain->nodes[i]->freq;
    qsort(freqArray, numOfNodes, sizeof(float), cmpfuncD);
    for (unsigned int j = 0; j < numOfNodes; j++)
    {
        for (i = 0; i < numOfNodes; i++)
        {
            if (freqArray[j] == eqgain->nodes[i]->freq)
            {
                float tmpFreq1 = eqgain->nodes[j]->freq;
                float tmpGain1 = eqgain->nodes[j]->gain;
                float tmpFreq2 = eqgain->nodes[i]->freq;
                float tmpGain2 = eqgain->nodes[i]->gain;
                eqgain->nodes[i]->freq = tmpFreq1;
                eqgain->nodes[i]->gain = tmpGain1;
                eqgain->nodes[j]->freq = tmpFreq2;
                eqgain->nodes[j]->gain = tmpGain2;
            }
        }
    }
    free(freqArray);
}
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, float freq, float gain, int sortNodes)
{
    if (!eqgain)
        return -1;
    if (!eqgain->nodesCount)
    {
        eqgain->nodes = (EqNode**)malloc((eqgain->nodesCount + 1) * sizeof(EqNode*));
        eqgain->nodes[0] = (EqNode*)malloc(sizeof(EqNode));
        eqgain->nodes[0]->freq = freq;
        eqgain->nodes[0]->gain = gain;
        eqgain->nodesCount = 1;
		return 1;
    }
    EqNode **tmpNode = (EqNode**)realloc(eqgain->nodes, (eqgain->nodesCount + 1) * sizeof(EqNode*));
    if (!tmpNode)
    {
        ArbitraryEqFree(eqgain);
        return -1;
    }
    else
    {
        tmpNode[eqgain->nodesCount] = (EqNode*)malloc(sizeof(EqNode));
        tmpNode[eqgain->nodesCount]->freq = freq;
        tmpNode[eqgain->nodesCount]->gain = gain;
        eqgain->nodes = tmpNode;
        eqgain->nodesCount++;
		if (sortNodes)
			NodesSorter(eqgain);
    }
    return 1;
}
unsigned int ArbitraryEqFindNode(ArbitraryEq *eqgain, float freq)
{
	if (!eqgain)
		return -1;
	for (unsigned int i = 0; i < eqgain->nodesCount; i++)
	{
		if (freq == eqgain->nodes[i]->freq)
			return i;
	}
	return 0;
}
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, float freq, int sortNodes)
{
    if (!eqgain)
        return -1;
    if (!eqgain->nodes)
        return -2;
    unsigned int i, findResult = 0;
    for (i = 0; i < eqgain->nodesCount; i++)
    {
        if (eqgain->nodes[i]->freq == freq)
        {
            findResult = i;
            free(eqgain->nodes[i]);
            break;
        }
    }
    if (!findResult)
        return 0;
    else
    {
        for (i = findResult; i < eqgain->nodesCount; i++)
            eqgain->nodes[i] = eqgain->nodes[i + 1];
        EqNode **tmpNode = (EqNode**)realloc(eqgain->nodes, (eqgain->nodesCount - 1) * sizeof(EqNode*));
        if (!tmpNode)
        {
            ArbitraryEqFree(eqgain);
            return -1;
        }
        else
        {
            eqgain->nodes = tmpNode;
            eqgain->nodesCount--;
    		if (sortNodes)
    			NodesSorter(eqgain);
        }
    }
    return 1;
}
void ArbitraryEqString2SortedNodes(ArbitraryEq *eqgain, char *frArbitraryEqString)
{
    if (!eqgain)
        return;
    unsigned int numOfNodes;
    char *p = frArbitraryEqString;
    char *counter = frArbitraryEqString;
    unsigned int i, count = 0;
    float number;
    while (*p)
    {
        if (get_float(p, &number))
        {
            strtod(p, &p);
            count++;
        }
        else
            p++;
    }
    count /= 2;
    numOfNodes = count;
    EqNode **nodes;
    if (numOfNodes != eqgain->nodesCount)
    {
        if (eqgain->nodes)
            EqNodesFree(eqgain);
        nodes = (EqNode**)malloc(count * sizeof(EqNode*));
        for (i = 0; i < count; i++)
            nodes[i] = (EqNode*)malloc(sizeof(EqNode));
    }
    else
        nodes = eqgain->nodes;
    i = 0;
    count = 1;
    while (*counter)
    {
        if (get_float(counter, &number))
        {
            count++;
            float val = strtod(counter, &counter);
			if (i == numOfNodes)
				break;
            if (count % 2)
            {
                nodes[i]->gain = val;
                i++;
            }
            else
                nodes[i]->freq = val;
        }
        else
            counter++;
    }
    eqgain->nodesCount = numOfNodes;
    eqgain->nodes = nodes;
    NodesSorter(eqgain);
}
