#ifndef ARBFIRGEN_H
#define ARBFIRGEN_H
#include "kissfft/kiss_fft.h"
double* fir2(int *nn, double *ff, double *aa, int ffsz);
typedef struct str_EqNodes
{
	double freq;
	double gain;
} EqNode;
typedef struct str_ArbitraryEq
{
	EqNode **nodes;
	unsigned int nodesCount;
	unsigned int isLinearPhase;
	unsigned int filterLength;
	unsigned int fLMul2;
	kiss_fft_cpx* timeData;
	kiss_fft_cpx* freqData;
	kiss_fft_cfg planForward;
	kiss_fft_cfg planReverse;
	double *impulseResponse;
	double* (*GetFilter)(struct str_ArbitraryEq*, double);
} ArbitraryEq;
void InitArbitraryEq(ArbitraryEq* eqgain, int *filterLength, int isLinearPhase);
void ArbitraryEqFree(ArbitraryEq *eqgain);
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, double freq, double gain, int sortNodes);
unsigned int ArbitraryEqFindNode(ArbitraryEq *eqgain, double freq);
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, double freq, int sortNodes);
void ArbitraryEqString2SortedNodes(ArbitraryEq *eqgain, char *frArbitraryEqString);
#endif