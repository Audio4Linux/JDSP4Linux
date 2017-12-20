#ifndef __AUTOCONVOLVER_H__
#define __AUTOCONVOLVER_H__
typedef struct str_AutoConvolver
{
    int methods, channels, hnShortLen, bufpos;
    double *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolver*, double**, double**, int);
} AutoConvolver;
typedef struct str_AutoConvolverMono
{
    int methods, hnShortLen, bufpos;
    double *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolverMono*, double*, double*, int);
} AutoConvolverMono;
int ACFFTWThreadInit();
void ACFFTWClean(int threads);
AutoConvolver* InitAutoConvolver(double **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, double **recommendation, int items, int fftwThreads);
AutoConvolver* InitAutoConvolverZeroLatency(double **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, int fftwThreads);
AutoConvolverMono* InitAutoConvolverMono(double *impulseResponse, int hlen, int audioBufferSize, double **recommendation, int items, int fftwThreads);
AutoConvolverMono* AllocateAutoConvolverMonoZeroLatency(double *impulseResponse, int hlen, int audioBufferSize, int fftwThreads);
void UpdateAutoConvolverMonoZeroLatency(AutoConvolverMono *autoConv, double *impulseResponse, int hlen);
void AutoConvolverFree(AutoConvolver *autoConv);
void AutoConvolverMonoFree(AutoConvolverMono *autoConv);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif