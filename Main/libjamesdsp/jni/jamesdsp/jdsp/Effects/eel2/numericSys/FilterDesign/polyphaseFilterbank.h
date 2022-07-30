#ifndef __POLYPHASEFILTERBANK_H__
#define __POLYPHASEFILTERBANK_H__
#include <stdlib.h>
#define DENORMAL_BUFFER (128)
typedef struct
{
	unsigned int N, m, L, N2;
	float *subbandData, *freqLabel;
	float *channelMatrix, *h;
	float *allpass_delay_chain, *virtualHilbertTransformDelay;
	float *APC_delay_1, *APC_delay_2, *Xk2;
	unsigned int *Sk, *decimationCounter;
	float alpha, postGain;
	// Denormal buster
	float noiseBuffer[DENORMAL_BUFFER];
	unsigned int noiseLoop;
} WarpedPFB;
size_t getMemSizeWarpedPFB(unsigned int N, unsigned int m);
void initWarpedPFB(WarpedPFB *pfb, double fs, unsigned int N, unsigned int m);
void assignPtrWarpedPFB(WarpedPFB *pfb, unsigned int N, unsigned int m);
void changeWarpingFactorWarpedPFB(WarpedPFB *pfb, float fs, float pfb_log_grid_den);
float* getPhaseCorrFilterWarpedPFB(WarpedPFB *pfb, float phCorrAlpha, unsigned int *CFiltLen);
#endif