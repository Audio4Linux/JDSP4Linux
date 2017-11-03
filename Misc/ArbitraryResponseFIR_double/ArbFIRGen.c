#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ArbFIRGen.h"
#define PI 3.141592653589793
#define PI2 6.283185307179586
int get_double(char *val, double *F)
{
	char *eptr;
	errno = 0;
	double f = strtod(val, &eptr);
	if (eptr != val && errno != ERANGE)
	{
		*F = f;
		return 1;
	}
	return 0;
}
int cmpfuncD(const void *a, const void *b)
{
	if (*(double*)a - *(double*)b < 0)
		return -1;
	if (*(double*)a - *(double*)b > 0)
		return 1;
	return 0;
}
int lower_bound(EqNode **array, int size, double key)
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
void winKaiser(double *win, int N, double alpha)
{
	int i, k, minus1 = N - 1;
	for (i = 0, k = minus1; i <= k; ++i, --k)
	{
		win[i] = getKaiser((double)(2 * i - minus1) / minus1, alpha);
		win[k] = win[i];
	}
}
void winKaiserHalf(double *win, int N, double alpha)
{
	int i, mulN = (N << 1) - 1;
	for (i = 0; i < N; i++)
		win[N - 1 - i] = getKaiser((double)(2 * i - mulN) / mulN, alpha);
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
//   double fs = 48000;
//   double fc = 120.0;
//   double gain = 5.0;
//   double freq[4] = { 0 / fs, (fc * 2.0) / fs, (fc * 2.0 + 80.0) / fs , 1.0 };
//   double amplitude[4] = { gain, gain, 0, 0 };
//   double *freqSamplImp;
//   freqSamplImp = fir2(&size, freq, amplitude, 4);
//   .......
//   free(freqSamplImp);
//   FIR2 uses a Hamming window.
inline void diff(double *y, double *f, int sz)
{
	--sz;
	for (int i = 0; i < sz; i++)
		f[i] = y[i + 1] - y[i];
}
inline int isneg(double *y, int sz)
{
	for (int i = 0; i < sz; i++)
		if (y[i] < 0) return 1;
	return 0;
}
double* fir2(int *nn, double *ff, double *aa, int ffsz)
{
	int npt, lap, npt2;
	// Convert gain to linear scale
	for (lap = 0; lap < ffsz; lap++)
		aa[lap] = pow(10.0, aa[lap] / 20.0);
	if (!(*nn % 2))
		*nn += 1;
	if (*nn < 1024)
		npt = 512;
	else
		npt = (int)pow(2.0, ceil(log((double)*nn) / log(2.0)));
	lap = (int)(npt / 25);
	if (fabs(ff[0]) > 0 || fabs(ff[ffsz - 1] - 1) > 1)
	{
		printf("The first frequency must be 0 and the last 1");
		return 0;
	}
	// Interpolate breakpoints onto large grid
	int nint = ffsz - 1;
	double *df = (double*)malloc(sizeof(double)*(ffsz - 1));
	diff(ff, df, ffsz);
	if (isneg(df, ffsz - 1))
	{
		printf("Frequencies must be non-decreasing");
		return 0;
	}
	npt = npt + 1; // Length of [dc 1 2 ... nyquist] frequencies.
	npt2 = npt * 2;
	int nb = 0;
	int ne = 0;
	double inc;
	double *H = (double*)malloc(sizeof(double)*npt);
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
			printf("Too abrupt an amplitude change near end of frequency interval");
			return 0;
		}
		int j;
		for (j = nb; j <= ne; j++)
		{
			if (nb == ne)
				inc = 0;
			else
				inc = (double)(j - nb) / (double)(ne - nb);
			H[j] = inc*aa[i + 1] + (1.0 - inc)*aa[i];
		}
		nb = ne + 1;
	}
	// Fourier time-shift.
	double dt = 0.5 * (double)(*nn - 1);
	fftw_complex *Hz2 = (fftw_complex*)fftw_malloc(npt * sizeof(fftw_complex));
	fftw_complex *Hz = (fftw_complex*)fftw_malloc(npt2 * sizeof(fftw_complex));
	for (i = 0; i < npt; i++)
	{
		double rad = -dt * PI * (double)i / ((double)(npt - 1));
		double Hz1Real = H[i] * cos(rad);
		double Hz1Imag = H[i] * sin(rad);
		Hz2[npt - 1 - i][1] = -1.0 * Hz1Imag;
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
	double *fo = (double*)fftw_malloc(sizeof(double) * npt2);
	fftw_plan plan = fftw_plan_dft_c2r_1d(nfft, Hz, fo, FFTW_ESTIMATE);
	fftw_execute(plan);
	inc = (double)(*nn - 1);
	double kfft = 1. / nfft;
	double *retArray = (double*)malloc(*nn * sizeof(double));
	for (i = 0; i < *nn; i++)
		retArray[i] = (0.54 - (0.46*cos(PI2*(double)i / inc))) * fo[i] * kfft;
	fftw_destroy_plan(plan);
	fftw_free(Hz);
	fftw_free(fo);
	fftw_free(Hz2);
	free(df);
	free(H);
	return retArray;
}
/////////////////////////////////////////////////////////////////////////////
// Log grid interpolated arbitrary response FIR filter design
/////////////////////////////////////////////////////////////////////////////
void minimumPhaseSpectrum(fftw_complex* timeData, fftw_complex* freqData, fftw_plan planForward, fftw_plan planReverse, unsigned int filterLength)
{
	unsigned int i, fLMul2 = filterLength << 1;
	double threshold = pow(10.0, -100.0 / 20.0);
	double logThreshold = log(threshold);
	for (i = 0; i < fLMul2; i++)
	{
		if (freqData[i][0] < threshold)
			freqData[i][0] = logThreshold;
		else
			freqData[i][0] = log(freqData[i][0]);
		freqData[i][1] = 0;
	}
	fftw_execute(planReverse);
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
	fftw_execute(planForward);
	for (i = 0; i < fLMul2; i++)
	{
		double eR = exp(freqData[i][0]);
		freqData[i][0] = eR * cos(freqData[i][1]);
		freqData[i][1] = eR * sin(freqData[i][1]);
	}
}
inline double gainAtLogGrid(ArbitraryEq *gains, double freq)
{
	double dbGain = 0.0, logLeft, logRightMinusLeft;
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
			double tmpfreq = gains->nodes[pos - 1]->freq;
			if (tmpfreq < 2.0)
				logLeft = tmpfreq;
			else
				logLeft = log(tmpfreq);
			tmpfreq = gains->nodes[pos]->freq;
			if (tmpfreq < 2.0)
				logRightMinusLeft = tmpfreq - logLeft;
			else
				logRightMinusLeft = log(tmpfreq) - logLeft;
			double t;
			if (freq < 2.0)
				t = (freq - logLeft) / logRightMinusLeft;
			else
				t = (log(freq) - logLeft) / logRightMinusLeft;
			if (gains->nodes[pos - 1]->gain == gains->nodes[pos]->gain)
				dbGain = gains->nodes[pos]->gain;
			else
				dbGain = gains->nodes[pos - 1]->gain + t * (gains->nodes[pos]->gain - gains->nodes[pos - 1]->gain);
		}
	}
	return dbGain;
}
double *ArbitraryEqMinimumPhase(ArbitraryEq *gains, double fs)
{
	unsigned int fLMul2 = gains->fLMul2;
	unsigned int flMul2Minus1 = fLMul2 - 1;
	fftw_complex* timeData = gains->timeData;
	fftw_complex* freqData = gains->freqData;
	fftw_plan planForward = gains->planForward;
	fftw_plan planReverse = gains->planReverse;
	// Log grid interpolation
	unsigned int i;
	for (i = 0; i < gains->filterLength; i++)
	{
		double freq = (double)i * fs / (double)fLMul2;
		double dbGain = gainAtLogGrid(gains, freq);
		double gain = pow(10.0, dbGain / 20.0);
		//		printf("%d: %lf\n", i, dbGain);
		freqData[i][0] = gain;
		freqData[i][1] = 0;
		freqData[flMul2Minus1 - i][0] = gain;
		freqData[flMul2Minus1 - i][1] = 0;
	}
	minimumPhaseSpectrum(timeData, freqData, planForward, planReverse, gains->filterLength);
	fftw_execute(planReverse);
	double factor, *finalImpulse = gains->impulseResponse;
	for (i = 0; i < gains->filterLength; i++)
	{
		factor = 0.5 * (1.0 + cos(PI2 * (double)i / (double)fLMul2));
		finalImpulse[i] = factor / fLMul2 * timeData[i][0];
	}
	return finalImpulse;
}
double *ArbitraryEqLinearPhase(ArbitraryEq *gains, double fs)
{
	unsigned int fLMul2 = gains->fLMul2;
	unsigned int flMul2Minus1 = fLMul2 - 1;
	fftw_complex* timeData = gains->timeData;
	fftw_complex* freqData = gains->freqData;
	fftw_plan planReverse = gains->planReverse;
	// Log grid interpolation
	unsigned int i;
	for (i = 0; i < gains->filterLength; i++)
	{
		double freq = (double)i * fs / (double)fLMul2;
		double dbGain = gainAtLogGrid(gains, freq);
		double gain = pow(10.0, dbGain / 20.0);
		freqData[i][0] = gain;
		freqData[i][1] = 0;
		freqData[flMul2Minus1 - i][0] = gain;
		freqData[flMul2Minus1 - i][1] = 0;
	}
	fftw_execute(planReverse);
	double *finalImpulse = gains->impulseResponse;
	flMul2Minus1 = gains->filterLength - 1;
	for (i = 0; i < gains->filterLength; i++)
	{
		double timeIntermediate = timeData[i][0] * 0.5 * (1.0 + cos(PI2 * (double)i / (double)fLMul2)) / (double)fLMul2;
		finalImpulse[flMul2Minus1 - i] = finalImpulse[flMul2Minus1 + i] = timeIntermediate;
	}
	return finalImpulse;
}
void InitArbitraryEq(ArbitraryEq *eqgain, int *filterLength, int isLinearPhase)
{
	eqgain->fLMul2 = *filterLength << 1;
	eqgain->timeData = fftw_alloc_complex(eqgain->fLMul2);
	eqgain->freqData = fftw_alloc_complex(eqgain->fLMul2);
	eqgain->filterLength = *filterLength;
	eqgain->isLinearPhase = isLinearPhase;
	eqgain->nodesCount = 0;
	if (!isLinearPhase)
	{
		eqgain->planForward = fftw_plan_dft_1d(eqgain->fLMul2, eqgain->timeData, eqgain->freqData, FFTW_FORWARD, FFTW_ESTIMATE);
		eqgain->impulseResponse = (double*)malloc(*filterLength * sizeof(double));
		eqgain->GetFilter = &ArbitraryEqMinimumPhase;
	}
	else
	{
		int minus1 = eqgain->fLMul2 - 1;
		eqgain->impulseResponse = (double*)malloc(minus1 * sizeof(double));
		eqgain->GetFilter = &ArbitraryEqLinearPhase;
		*filterLength = minus1;
	}
	eqgain->planReverse = fftw_plan_dft_1d(eqgain->fLMul2, eqgain->freqData, eqgain->timeData, FFTW_BACKWARD, FFTW_ESTIMATE);
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
		fftw_destroy_plan(eqgain->planForward);
	fftw_free(eqgain->freqData);
	fftw_destroy_plan(eqgain->planReverse);
	free(eqgain->impulseResponse);
	free(eqgain);
	eqgain = 0;
}
void NodesSorter(ArbitraryEq *eqgain)
{
	unsigned int i, numOfNodes = eqgain->nodesCount;
	double *freqArray = (double*)malloc(numOfNodes * sizeof(double));
	for (i = 0; i < numOfNodes; i++)
		freqArray[i] = eqgain->nodes[i]->freq;
	qsort(freqArray, numOfNodes, sizeof(double), cmpfuncD);
	for (unsigned int j = 0; j < numOfNodes; j++)
	{
		for (i = 0; i < numOfNodes; i++)
		{
			if (freqArray[j] == eqgain->nodes[i]->freq)
			{
				double tmpFreq1 = eqgain->nodes[j]->freq;
				double tmpGain1 = eqgain->nodes[j]->gain;
				double tmpFreq2 = eqgain->nodes[i]->freq;
				double tmpGain2 = eqgain->nodes[i]->gain;
				eqgain->nodes[i]->freq = tmpFreq1;
				eqgain->nodes[i]->gain = tmpGain1;
				eqgain->nodes[j]->freq = tmpFreq2;
				eqgain->nodes[j]->gain = tmpGain2;
			}
		}
	}
	free(freqArray);
}
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, double freq, double gain, int sortNodes)
{
	if (!eqgain)
		return -1;
	if (!eqgain->nodesCount)
	{
		eqgain->nodes = (EqNode**)malloc((eqgain->nodesCount + 1) * sizeof(EqNode*));
		if (!eqgain->nodes)
			return -2;
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
unsigned int ArbitraryEqFindNode(ArbitraryEq *eqgain, double freq)
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
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, double freq, int sortNodes)
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
	double number;
	while (*p)
	{
		if (get_double(p, &number))
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
		if (get_double(counter, &number))
		{
			count++;
			double val = strtod(counter, &counter);
			if (count % 2)
			{
				nodes[i]->gain = val;
				i++;
			}
			else
			{
				nodes[i]->freq = val;
			}
		}
		else
			counter++;
	}
	eqgain->nodesCount = numOfNodes;
	eqgain->nodes = nodes;
	printf("Unsorted\n");
	for (i = 0; i < numOfNodes; i++)
		printf("Node[%d]: Freq=%lf, Gain=%lf\n", i, nodes[i]->freq, nodes[i]->gain);
	NodesSorter(eqgain);
	printf("Sorted\n");
	for (i = 0; i < numOfNodes; i++)
		printf("Node[%d]: Freq=%lf, Gain=%lf\n", i, nodes[i]->freq, nodes[i]->gain);
}