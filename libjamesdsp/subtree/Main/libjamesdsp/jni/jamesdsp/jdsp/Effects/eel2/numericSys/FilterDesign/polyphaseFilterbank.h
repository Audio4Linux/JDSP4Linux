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
void analysisWarpedPFB(WarpedPFB *pfb, float x);
void writeSubbandDatWarpedPFB(WarpedPFB *pfb, float *subbands);
float synthesisWarpedPFB(WarpedPFB *pfb);
void analysisWarpedPFBStereo(WarpedPFB *pfb1, WarpedPFB *pfb2, float *x1, float *x2);
void getSubbandDatWarpedPFBStereo(WarpedPFB *pfb1, WarpedPFB *pfb2, float *subbands1, float *subbands2, float *curSk);
void getSubbandDatWarpedPFB(WarpedPFB *pfb, float *subbands, float *curSk);
void writeSubbandDatWarpedPFBStereo(WarpedPFB *pfb1, WarpedPFB *pfb2, float *subbands1, float *subbands2);
#define getMemSizeWarpedPFB(N, m) (sizeof(WarpedPFB) + (N * sizeof(unsigned int)) + ((N + 1) * sizeof(float)) + (N * sizeof(float)) + ((2 * m * N) * sizeof(float)) + ((2 * m * N) * sizeof(float)) + (N * 2 * N * sizeof(float)) + (N * sizeof(unsigned int)) + (N * sizeof(float)) + ((2 * m * N) * sizeof(float)) + ((2 * m * N) * sizeof(float)) + (2 * m * N * sizeof(float)))
void synthesisWarpedPFBStereo(WarpedPFB *pfb1, WarpedPFB *pfb2, float *y1, float *y2);
void initWarpedPFB(WarpedPFB *pfb, double fs, unsigned int N, unsigned int m);
void assignPtrWarpedPFB(WarpedPFB *pfb, unsigned int N, unsigned int m);
void changeWarpingFactorWarpedPFB(WarpedPFB *pfb, float fs, float pfb_log_grid_den);
float* getPhaseCorrFilterWarpedPFB(WarpedPFB *pfb, float phCorrAlpha, unsigned int *CFiltLen);
#endif