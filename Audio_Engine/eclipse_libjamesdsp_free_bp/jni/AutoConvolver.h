#ifndef __AUTOCONVOLVER_H__
#define __AUTOCONVOLVER_H__
typedef struct str_AutoConvolver1x1
{
    int methods, hnShortLen, bufpos;
    double *inbuf, *outbuf;
    void *filter;
    void(*process)(struct str_AutoConvolver1x1*, double*, double*, int);
} AutoConvolver1x1;
AutoConvolver1x1* InitAutoConvolver1x1(double *impulseResponse, int hlen, int audioBufferSize, double gaindB, double **recommendation, int items);
AutoConvolver1x1* AllocateAutoConvolver1x1ZeroLatency(double *impulseResponse, int hlen, int audioBufferSize);
void UpdateAutoConvolver1x1ZeroLatency(AutoConvolver1x1 *autoConv, double *impulseResponse, int hlen);
void AutoConvolver1x1Free(AutoConvolver1x1 *autoConv);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif
