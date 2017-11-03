#ifndef ARBFIRGEN_H
#define ARBFIRGEN_H
#include "fftw3.h"
float* fir2(int *nn, float *ff, float *aa, int ffsz);
typedef struct str_EqNodes
{
    float freq;
    float gain;
} EqNode;
typedef struct str_ArbitraryEq
{
    EqNode **nodes;
    unsigned int nodesCount;
    unsigned int isLinearPhase;
    unsigned int filterLength;
    unsigned int fLMul2;
    fftwf_complex* timeData;
    fftwf_complex* freqData;
    fftwf_plan planForward;
    fftwf_plan planReverse;
    float *impulseResponse;
    float* (*GetFilter)(struct str_ArbitraryEq*, float);
} ArbitraryEq;
void InitArbitraryEq(ArbitraryEq* eqgain, int *filterLength, int isLinearPhase);
void ArbitraryEqFree(ArbitraryEq *eqgain);
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, float freq, float gain, int sortNodes);
unsigned int ArbitraryEqFindNode(ArbitraryEq *eqgain, float freq);
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, float freq, int sortNodes);
void ArbitraryEqString2SortedNodes(ArbitraryEq *eqgain, char *frArbitraryEqString);
#endif
