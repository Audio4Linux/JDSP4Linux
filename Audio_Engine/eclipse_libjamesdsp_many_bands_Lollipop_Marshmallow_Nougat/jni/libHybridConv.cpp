/***************************************************************************
 *   Copyright (C) 2009 by Christian Borss                                 *
 *   christian.borss@rub.de                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#if defined(HYBRIDCONV_USE_SSE)
#include <xmmintrin.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libHybridConv.h"
void hcProcess(HConvSingle *filter, float *x, float *y)
{
    int j, flen, size, mpos, s, n, start, stop;
#if defined(HYBRIDCONV_USE_SSE)
	int flen4;
#endif
	float *out, *hist;
    flen = filter->framelength;
    size = sizeof(float) * flen;
    memcpy(filter->dft_time, x, size);
    memset(&(filter->dft_time[flen]), 0, size);
    fftwf_execute(filter->fft);
    for (j = 0; j < flen + 1; j++)
    {
        filter->in_freq_real[j] = filter->dft_freq[j][0];
        filter->in_freq_imag[j] = filter->dft_freq[j][1];
    }
#if defined(HYBRIDCONV_USE_SSE)
	__m128 *x4_real;
	__m128 *x4_imag;
	__m128 *h4_real;
	__m128 *h4_imag;
	__m128 *y4_real;
	__m128 *y4_imag;
	float *x_real;
	float *x_imag;
	float *h_real;
	float *h_imag;
	float *y_real;
	float *y_imag;
	flen = filter->framelength;
	x_real = filter->in_freq_real;
	x_imag = filter->in_freq_imag;
	x4_real = (__m128*)x_real;
	x4_imag = (__m128*)x_imag;
	start = filter->steptask[filter->step];
	stop = filter->steptask[filter->step + 1];
	for (s = start; s < stop; s++)
	{
		n = (s + filter->mixpos) % filter->num_mixbuf;
		y_real = filter->mixbuf_freq_real[n];
		y_imag = filter->mixbuf_freq_imag[n];
		y4_real = (__m128*)y_real;
		y4_imag = (__m128*)y_imag;
		h_real = filter->filterbuf_freq_real[s];
		h_imag = filter->filterbuf_freq_imag[s];
		h4_real = (__m128*)h_real;
		h4_imag = (__m128*)h_imag;
		flen4 = flen / 4;
		for (n = 0; n < flen4; n++)
		{
			__m128 a = _mm_mul_ps(x4_real[n], h4_real[n]);
			__m128 b = _mm_mul_ps(x4_imag[n], h4_imag[n]);
			__m128 c = _mm_sub_ps(a, b);
			y4_real[n] = _mm_add_ps(y4_real[n], c);
			a = _mm_mul_ps(x4_real[n], h4_imag[n]);
			b = _mm_mul_ps(x4_imag[n], h4_real[n]);
			c = _mm_add_ps(a, b);
			y4_imag[n] = _mm_add_ps(y4_imag[n], c);
		}
		y_real[flen] += x_real[flen] * h_real[flen] -
			x_imag[flen] * h_imag[flen];
		y_imag[flen] += x_real[flen] * h_imag[flen] +
			x_imag[flen] * h_real[flen];
	}
	filter->step = (filter->step + 1) % filter->maxstep;
#else
	float *x_real;
	float *x_imag;
	float *h_real;
	float *h_imag;
	float *y_real;
	float *y_imag;
	flen = filter->framelength;
	x_real = filter->in_freq_real;
	x_imag = filter->in_freq_imag;
	start = filter->steptask[filter->step];
	stop = filter->steptask[filter->step + 1];
	for (s = start; s < stop; s++)
	{
		n = (s + filter->mixpos) % filter->num_mixbuf;
		y_real = filter->mixbuf_freq_real[n];
		y_imag = filter->mixbuf_freq_imag[n];
		h_real = filter->filterbuf_freq_real[s];
		h_imag = filter->filterbuf_freq_imag[s];
		for (n = 0; n < flen + 1; n++)
		{
			y_real[n] += x_real[n] * h_real[n] -
				x_imag[n] * h_imag[n];
			y_imag[n] += x_real[n] * h_imag[n] +
				x_imag[n] * h_real[n];
		}
	}
	filter->step = (filter->step + 1) % filter->maxstep;
#endif
	flen = filter->framelength;
	mpos = filter->mixpos;
	out = filter->dft_time;
	hist = filter->history_time;
	for (j = 0; j < flen + 1; j++)
	{
		filter->dft_freq[j][0] = filter->mixbuf_freq_real[mpos][j];
		filter->dft_freq[j][1] = filter->mixbuf_freq_imag[mpos][j];
		filter->mixbuf_freq_real[mpos][j] = 0.0;
		filter->mixbuf_freq_imag[mpos][j] = 0.0;
	}
	fftwf_execute(filter->ifft);
	for (n = 0; n < flen; n++)
		y[n] = out[n] + hist[n];
	size = sizeof(float) * flen;
	memcpy(hist, &(out[flen]), size);
	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}
void hcProcessAdd(HConvSingle *filter, float *x, float *y)
{
	int j, flen, size, mpos, s, n, start, stop;
#if defined(HYBRIDCONV_USE_SSE)
	int flen4;
#endif
	float *out, *hist;
	flen = filter->framelength;
	size = sizeof(float) * flen;
	memcpy(filter->dft_time, x, size);
	memset(&(filter->dft_time[flen]), 0, size);
	fftwf_execute(filter->fft);
	for (j = 0; j < flen + 1; j++)
	{
		filter->in_freq_real[j] = filter->dft_freq[j][0];
		filter->in_freq_imag[j] = filter->dft_freq[j][1];
	}
#if defined(HYBRIDCONV_USE_SSE)
	__m128 *x4_real;
	__m128 *x4_imag;
	__m128 *h4_real;
	__m128 *h4_imag;
	__m128 *y4_real;
	__m128 *y4_imag;
	float *x_real;
	float *x_imag;
	float *h_real;
	float *h_imag;
	float *y_real;
	float *y_imag;
	flen = filter->framelength;
	x_real = filter->in_freq_real;
	x_imag = filter->in_freq_imag;
	x4_real = (__m128*)x_real;
	x4_imag = (__m128*)x_imag;
	start = filter->steptask[filter->step];
	stop = filter->steptask[filter->step + 1];
	for (s = start; s < stop; s++)
	{
		n = (s + filter->mixpos) % filter->num_mixbuf;
		y_real = filter->mixbuf_freq_real[n];
		y_imag = filter->mixbuf_freq_imag[n];
		y4_real = (__m128*)y_real;
		y4_imag = (__m128*)y_imag;
		h_real = filter->filterbuf_freq_real[s];
		h_imag = filter->filterbuf_freq_imag[s];
		h4_real = (__m128*)h_real;
		h4_imag = (__m128*)h_imag;
		flen4 = flen / 4;
		for (n = 0; n < flen4; n++)
		{
			__m128 a = _mm_mul_ps(x4_real[n], h4_real[n]);
			__m128 b = _mm_mul_ps(x4_imag[n], h4_imag[n]);
			__m128 c = _mm_sub_ps(a, b);
			y4_real[n] = _mm_add_ps(y4_real[n], c);
			a = _mm_mul_ps(x4_real[n], h4_imag[n]);
			b = _mm_mul_ps(x4_imag[n], h4_real[n]);
			c = _mm_add_ps(a, b);
			y4_imag[n] = _mm_add_ps(y4_imag[n], c);
		}
		y_real[flen] += x_real[flen] * h_real[flen] -
			x_imag[flen] * h_imag[flen];
		y_imag[flen] += x_real[flen] * h_imag[flen] +
			x_imag[flen] * h_real[flen];
	}
	filter->step = (filter->step + 1) % filter->maxstep;
#else
	float *x_real;
	float *x_imag;
	float *h_real;
	float *h_imag;
	float *y_real;
	float *y_imag;
	flen = filter->framelength;
	x_real = filter->in_freq_real;
	x_imag = filter->in_freq_imag;
	start = filter->steptask[filter->step];
	stop = filter->steptask[filter->step + 1];
	for (s = start; s < stop; s++)
	{
		n = (s + filter->mixpos) % filter->num_mixbuf;
		y_real = filter->mixbuf_freq_real[n];
		y_imag = filter->mixbuf_freq_imag[n];
		h_real = filter->filterbuf_freq_real[s];
		h_imag = filter->filterbuf_freq_imag[s];
		for (n = 0; n < flen + 1; n++)
		{
			y_real[n] += x_real[n] * h_real[n] -
				x_imag[n] * h_imag[n];
			y_imag[n] += x_real[n] * h_imag[n] +
				x_imag[n] * h_real[n];
		}
	}
	filter->step = (filter->step + 1) % filter->maxstep;
#endif
	flen = filter->framelength;
	mpos = filter->mixpos;
	out = filter->dft_time;
	hist = filter->history_time;
	for (j = 0; j < flen + 1; j++)
	{
		filter->dft_freq[j][0] = filter->mixbuf_freq_real[mpos][j];
		filter->dft_freq[j][1] = filter->mixbuf_freq_imag[mpos][j];
		filter->mixbuf_freq_real[mpos][j] = 0.0;
		filter->mixbuf_freq_imag[mpos][j] = 0.0;
	}
	fftwf_execute(filter->ifft);
	for (n = 0; n < flen; n++)
		y[n] += out[n] + hist[n];
	size = sizeof(float) * flen;
	memcpy(hist, &(out[flen]), size);
	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}

void hcInitSingle(HConvSingle *filter, float *h, int hlen, int flen, int steps, int fftOptimize)
{
    int i, j, size, num, pos;
    float gain;
    // processing step counter
    filter->step = 0;
    // number of processing steps per audio frame
    filter->maxstep = steps;
    // current frame index
    filter->mixpos = 0;
    // number of samples per audio frame
    filter->framelength = flen;
    // DFT buffer (time domain)
    size = sizeof(float) * 2 * flen;
    filter->dft_time = (float *)fftwf_malloc(size);
    // DFT buffer (frequency domain)
    size = sizeof(fftwf_complex) * (flen + 1);
    filter->dft_freq = (fftwf_complex*)fftwf_malloc(size);
    // input buffer (frequency domain)
    size = sizeof(float) * (flen + 1);
    filter->in_freq_real = (float*)fftwf_malloc(size);
    filter->in_freq_imag = (float*)fftwf_malloc(size);
    // number of filter segments
    filter->num_filterbuf = (hlen + flen - 1) / flen;
    // processing tasks per step
    size = sizeof(int) * (steps + 1);
    filter->steptask = (int *)malloc(size);
    num = filter->num_filterbuf / steps;
    for (i = 0; i <= steps; i++)
        filter->steptask[i] = i * num;
    if (filter->steptask[1] == 0)
        pos = 1;
    else
        pos = 2;
    num = filter->num_filterbuf % steps;
    for (j = pos; j < pos + num; j++)
    {
        for (i = j; i <= steps; i++)
            filter->steptask[i]++;
    }
    // filter segments (frequency domain)
    size = sizeof(float*) * filter->num_filterbuf;
    filter->filterbuf_freq_real = (float**)fftwf_malloc(size);
    filter->filterbuf_freq_imag = (float**)fftwf_malloc(size);
    for (i = 0; i < filter->num_filterbuf; i++)
    {
        size = sizeof(float) * (flen + 1);
        filter->filterbuf_freq_real[i] = (float*)fftwf_malloc(size);
        filter->filterbuf_freq_imag[i] = (float*)fftwf_malloc(size);
    }
    // number of mixing segments
    filter->num_mixbuf = filter->num_filterbuf + 1;
    // mixing segments (frequency domain)
    size = sizeof(float*) * filter->num_mixbuf;
    filter->mixbuf_freq_real = (float**)fftwf_malloc(size);
    filter->mixbuf_freq_imag = (float**)fftwf_malloc(size);
    for (i = 0; i < filter->num_mixbuf; i++)
    {
        size = sizeof(float) * (flen + 1);
        filter->mixbuf_freq_real[i] = (float*)fftwf_malloc(size);
        filter->mixbuf_freq_imag[i] = (float*)fftwf_malloc(size);
        memset(filter->mixbuf_freq_real[i], 0, size);
        memset(filter->mixbuf_freq_imag[i], 0, size);
    }
    // history buffer (time domain)
    size = sizeof(float) * flen;
    filter->history_time = (float *)fftwf_malloc(size);
    memset(filter->history_time, 0, size);
    if (fftOptimize == 0)
        fftOptimize = FFTW_ESTIMATE;
    else if (fftOptimize == 1)
        fftOptimize = FFTW_MEASURE;
    else if (fftOptimize == 2)
        fftOptimize = FFTW_PATIENT;
    // FFT transformation plan
    filter->fft = fftwf_plan_dft_r2c_1d(2 * flen, filter->dft_time, filter->dft_freq, fftOptimize | FFTW_PRESERVE_INPUT);
    // IFFT transformation plan
    filter->ifft = fftwf_plan_dft_c2r_1d(2 * flen, filter->dft_freq, filter->dft_time, fftOptimize | FFTW_PRESERVE_INPUT);
    // generate filter segments
    gain = 0.5f / flen;
    size = sizeof(float) * 2 * flen;
    memset(filter->dft_time, 0, size);
    for (i = 0; i < filter->num_filterbuf - 1; i++)
    {
        for (j = 0; j < flen; j++)
            filter->dft_time[j] = gain * h[i * flen + j];
        fftwf_execute(filter->fft);
        for (j = 0; j < flen + 1; j++)
        {
            filter->filterbuf_freq_real[i][j] = filter->dft_freq[j][0];
            filter->filterbuf_freq_imag[i][j] = filter->dft_freq[j][1];
        }
    }
    for (j = 0; j < hlen - i * flen; j++)
        filter->dft_time[j] = gain * h[i * flen + j];
    size = sizeof(float) * ((i + 1) * flen - hlen);
    memset(&(filter->dft_time[hlen - i * flen]), 0, size);
    fftwf_execute(filter->fft);
    for (j = 0; j < flen + 1; j++)
    {
        filter->filterbuf_freq_real[i][j] = filter->dft_freq[j][0];
        filter->filterbuf_freq_imag[i][j] = filter->dft_freq[j][1];
    }
}

void hcCloseSingle(HConvSingle *filter)
{
    int i;
    fftwf_destroy_plan(filter->ifft);
    fftwf_destroy_plan(filter->fft);
    fftwf_free(filter->history_time);
    for (i = 0; i < filter->num_mixbuf; i++)
    {
        fftwf_free(filter->mixbuf_freq_real[i]);
        fftwf_free(filter->mixbuf_freq_imag[i]);
    }
    fftwf_free(filter->mixbuf_freq_real);
    fftwf_free(filter->mixbuf_freq_imag);
    for (i = 0; i < filter->num_filterbuf; i++)
    {
        fftwf_free(filter->filterbuf_freq_real[i]);
        fftwf_free(filter->filterbuf_freq_imag[i]);
    }
    fftwf_free(filter->filterbuf_freq_real);
    fftwf_free(filter->filterbuf_freq_imag);
    fftwf_free(filter->in_freq_real);
    fftwf_free(filter->in_freq_imag);
    fftwf_free(filter->dft_freq);
    fftwf_free(filter->dft_time);
    free(filter->steptask);
    memset(filter, 0, sizeof(HConvSingle));
}