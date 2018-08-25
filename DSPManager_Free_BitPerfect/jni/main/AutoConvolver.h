#ifndef __AUTOCONVOLVER_H__
#define __AUTOCONVOLVER_H__
typedef struct str_AutoConvolverMono
{
    int methods, hnShortLen, bufpos;
    double *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolverMono*, double*, double*, int);
} AutoConvolverMono;
int ACFFTWThreadInit();
void ACFFTWClean(int threads);
AutoConvolverMono* InitAutoConvolverMono(double *impulseResponse, int hlen, int audioBufferSize, double gaindB, double **recommendation, int items, int fftwThreads);
AutoConvolverMono* AllocateAutoConvolverMonoZeroLatency(double *impulseResponse, int hlen, int audioBufferSize, int fftwThreads);
void UpdateAutoConvolverMonoZeroLatency(AutoConvolverMono *autoConv, double *impulseResponse, int hlen);
void AutoConvolverMonoFree(AutoConvolverMono *autoConv);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif
