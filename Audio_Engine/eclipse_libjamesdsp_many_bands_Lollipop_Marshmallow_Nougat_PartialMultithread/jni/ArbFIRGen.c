#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "ArbFIRGen.h"
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
	int i, k, mulN = (N << 1) - 1;
	for (i = 0; i < N; i++)
		win[N - 1 - i] = (float)getKaiser((double)(2 * i - mulN) / mulN, alpha);
}
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
float *ArbitraryEqMinimumPhase(ArbitraryEq *gains, float fs, float attenuate)
{
    unsigned int fLMul2 = gains->fLMul2;
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
        freqData[fLMul2 - i - 1][0] = gain;
        freqData[fLMul2 - i - 1][1] = 0;
    }
    minimumPhaseSpectrum(timeData, freqData, planForward, planReverse, gains->filterLength);
    fftwf_execute(planReverse);
	float factor, *finalImpulse = gains->impulseResponse;
	winKaiserHalf(finalImpulse, gains->filterLength, (double)attenuate);
    for (i = 0; i < gains->filterLength; i++)
    {
        factor = (float)(0.5 * (1.0 + cos(6.283185307179586 * (double)i / fLMul2)));
		finalImpulse[i] *= factor / (float)fLMul2 * timeData[i][0];
    }
    return finalImpulse;
}
float *ArbitraryEqLinearPhase(ArbitraryEq *gains, float fs, float attenuate)
{
    unsigned int fLMul2 = gains->fLMul2, flMul2Minus1 = fLMul2 - 1;
    fftwf_complex* timeData = gains->timeData;
    fftwf_complex* freqData = gains->freqData;
    fftwf_plan planReverse = gains->planReverse;
    // Log grid interpolation
    unsigned int i;
    for (i = 0; i < gains->filterLength; i++)
    {
        float freq = (float)i * fs / (float)fLMul2;
        float dbGain = gainAtLogGrid(gains, freq);
        float gain = powf(10.0f, dbGain / 20.0f) / (float)fLMul2;
        freqData[i][0] = gain;
        freqData[i][1] = 0;
        freqData[fLMul2 - i - 1][0] = gain;
        freqData[fLMul2 - i - 1][1] = 0;
    }
    fftwf_execute(planReverse);
    float *finalImpulse = gains->impulseResponse;
	winKaiser(finalImpulse, flMul2Minus1, (double)attenuate);
	unsigned int minus1 = gains->filterLength - 1;
    for (i = 0; i < gains->filterLength; i++)
		finalImpulse[minus1 - i] = finalImpulse[minus1 + i] *= timeData[i][0];
    return finalImpulse;
}
ArbitraryEq* InitArbitraryEq(int *filterLength, int isLinearPhase)
{
    ArbitraryEq *eqgain = (ArbitraryEq*)calloc(1, sizeof(ArbitraryEq));
    if (!eqgain)
        return 0;
    eqgain->fLMul2 = *filterLength << 1;
    eqgain->timeData = fftwf_alloc_complex(eqgain->fLMul2);
    eqgain->freqData = fftwf_alloc_complex(eqgain->fLMul2);
    eqgain->filterLength = *filterLength;
    eqgain->isLinearPhase = isLinearPhase;
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
    if (!eqgain->impulseResponse)
    {
        free(eqgain);
        return 0;
    }
    return eqgain;
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
    free(eqgain);
    eqgain = 0;
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
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, float freq, float gain)
{
    if (!eqgain)
        return -1;
    if (!eqgain->nodes)
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
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, float freq)
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