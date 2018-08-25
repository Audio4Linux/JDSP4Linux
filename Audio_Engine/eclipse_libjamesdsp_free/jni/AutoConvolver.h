#ifndef __AUTOCONVOLVER_H__
#define __AUTOCONVOLVER_H__
typedef struct str_AutoConvolverMono
{
    int methods, hnShortLen, bufpos;
    float *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolverMono*, float*, float*, int);
} AutoConvolverMono;
int ACFFTWThreadInit();
void ACFFTWClean(int threads);
AutoConvolverMono* InitAutoConvolverMono(float *impulseResponse, int hlen, int audioBufferSize, float gaindB, double **recommendation, int items, int fftwThreads);
AutoConvolverMono* AllocateAutoConvolverMonoZeroLatency(float *impulseResponse, int hlen, int audioBufferSize, int fftwThreads);
void UpdateAutoConvolverMonoZeroLatency(AutoConvolverMono *autoConv, float *impulseResponse, int hlen);
void AutoConvolverMonoFree(AutoConvolverMono *autoConv);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif
