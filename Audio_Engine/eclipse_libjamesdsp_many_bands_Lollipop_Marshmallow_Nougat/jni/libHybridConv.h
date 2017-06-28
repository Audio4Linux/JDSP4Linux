#ifndef __LIBHYBRIDCONV_H__
#define __LIBHYBRIDCONV_H__
#include "fftw3.h"
typedef struct str_HConvSingle
{
    int step;
    int maxstep;
    int mixpos;
    int framelength;
    int frameLenMulFloatSize;
    int *steptask;
    float *dft_time;
    fftwf_complex *dft_freq;
    float *in_freq_real;
    float *in_freq_imag;
    int num_filterbuf;
    float **filterbuf_freq_real;
    float **filterbuf_freq_imag;
    int num_mixbuf;
    float **mixbuf_freq_real;
    float **mixbuf_freq_imag;
    float *history_time;
    fftwf_plan fft;
    fftwf_plan ifft;
} HConvSingle;
int hcFFTWThreadInit();
void hcFFTWThreadClean();
void hcProcess(HConvSingle *filter, float *x, float *y);
void hcProcessAdd(HConvSingle *filter, float *x, float *y);
void hcInitSingle(HConvSingle *filter, float *h, int hlen, int flen, int steps, int fftOptimize, int fftwThreads);
void hcCloseSingle(HConvSingle *filter);
#endif
