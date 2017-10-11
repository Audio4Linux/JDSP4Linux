#ifndef __LIBHYBRIDCONV_H__
#define __LIBHYBRIDCONV_H__
typedef struct str_AutoConvolver
{
    int methods, channels, hnShortLen, bufpos;
	int *routeIn, *routeOut;
    float *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolver*, float**, float**, int);
} AutoConvolver;
typedef struct str_AutoConvolverMono
{
	int methods, hnShortLen, bufpos;
	int *routeIn, *routeOut;
	float *inbuf, *outbuf;
	void *filter;
	void(*process)(struct str_AutoConvolverMono*, float*, float*, int);
} AutoConvolverMono;
int ACFFTWThreadInit();
void ACFFTWClean(int threads);
AutoConvolver* InitAutoConvolver(float **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, double **recommendation, int items, int fftwThreads);
AutoConvolver* InitAutoConvolverZeroLatency(float **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, int fftwThreads);
AutoConvolverMono* InitAutoConvolverMono(float *impulseResponse, int hlen, int audioBufferSize, double **recommendation, int items, int fftwThreads);
AutoConvolverMono* InitAutoConvolverMonoZeroLatency(float *impulseResponse, int hlen, int audioBufferSize, int fftwThreads);
void AutoConvolverFree(AutoConvolver *autoConv);
void AutoConvolverMonoFree(AutoConvolverMono *autoConv);
int PartitionerAnalyzer(int hlen, int latency, int strategy, int fs, int entriesResult, double **result_c0_c1, int *sflen_best, int *mflen_best, int *lflen_best);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif
