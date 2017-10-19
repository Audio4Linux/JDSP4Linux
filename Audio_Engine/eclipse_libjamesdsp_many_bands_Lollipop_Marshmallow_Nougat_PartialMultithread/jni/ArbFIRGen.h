#ifndef ARBFIRGEN_H
#define ARBFIRGEN_H
#include "fftw3.h"
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
    float* (*GetFilter)(struct str_ArbitraryEq*, float, float);
} ArbitraryEq;
ArbitraryEq* InitArbitraryEq(int *filterLength, int isLinearPhase);
void ArbitraryEqFree(ArbitraryEq *eqgain);
int ArbitraryEqInsertNode(ArbitraryEq *eqgain, float freq, float gain);
unsigned int ArbitraryEqFindNode(ArbitraryEq *eqgain, float freq);
int ArbitraryEqRemoveNode(ArbitraryEq *eqgain, float freq);
void ArbitraryEqString2SortedNodes(ArbitraryEq *eqgain, char *frArbitraryEqString);
#endif