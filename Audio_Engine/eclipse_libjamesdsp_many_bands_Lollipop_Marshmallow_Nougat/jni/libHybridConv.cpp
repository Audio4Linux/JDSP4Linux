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


void hcPutSingle(HConvSingle *filter, float *x)
{
	int j, flen, size;

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
}

void hcProcessSingle(HConvSingle *filter)
{
#if defined(HYBRIDCONV_USE_SSE)
	int s, n, start, stop, flen, flen4;
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
	int s, n, start, stop, flen;
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
	stop  = filter->steptask[filter->step + 1];
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
}


void hcGetSingle(HConvSingle *filter, float *y)
{
	int flen, mpos;
	float *out;
	float *hist;
	int size, n, j;

	flen = filter->framelength;
	mpos = filter->mixpos;
	out  = filter->dft_time;
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
	{
		y[n] = out[n] + hist[n];
	}
	size = sizeof(float) * flen;
	memcpy(hist, &(out[flen]), size);
	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}


void hcGetAddSingle(HConvSingle *filter, float *y)
{
	int flen, mpos;
	float *out;
	float *hist;
	int size, n, j;

	flen = filter->framelength;
	mpos = filter->mixpos;
	out  = filter->dft_time;
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
	{
		y[n] += out[n] + hist[n];
	}
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

void hcProcessDual(HConvDual *filter, float *in, float *out)
{
	int lpos, size, i;

	// convolution with short segments
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetSingle(filter->f_short, out);

	// add contribution from last long frame
	lpos = filter->step * filter->flen_short;
	for (i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_long[lpos + i];

	// convolution with long segments
	if (filter->step == 0)
		hcPutSingle(filter->f_long, filter->in_long);
	hcProcessSingle(filter->f_long);
	if (filter->step == filter->maxstep - 1)
		hcGetSingle(filter->f_long, filter->out_long);

	// add current frame to long input buffer
	lpos = filter->step * filter->flen_short;
	size = sizeof(float) * filter->flen_short;
	memcpy(&(filter->in_long[lpos]), in, size);

	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcProcessAddDual(HConvDual *filter, float *in, float *out)
{
	int lpos, size, i;

	// convolution with short segments
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetAddSingle(filter->f_short, out);

	// add contribution from last long frame
	lpos = filter->step * filter->flen_short;
	for (i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_long[lpos + i];

	// convolution with long segments
	if (filter->step == 0)
		hcPutSingle(filter->f_long, filter->in_long);
	hcProcessSingle(filter->f_long);
	if (filter->step == filter->maxstep - 1)
		hcGetSingle(filter->f_long, filter->out_long);

	// add current frame to long input buffer
	lpos = filter->step * filter->flen_short;
	size = sizeof(float) * filter->flen_short;
	memcpy(&(filter->in_long[lpos]), in, size);

	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcInitDual(HConvDual *filter, float *h, int hlen, int sflen, int lflen, int fftOptimize)
{
	int size;
	float *h2 = NULL;
	int h2len;

	// sanity check: minimum impulse response length
	h2len = 2 * lflen + 1;
	if (hlen < h2len)
	{
		size = sizeof(float) * h2len;
		h2 = (float*)fftwf_malloc(size);
		memset(h2, 0, size);
		size = sizeof(float) * hlen;
		memcpy(h2, h, size);
		h = h2;
		hlen = h2len;
	}

	// processing step counter
	filter->step = 0;

	// number of processing steps per long audio frame
	filter->maxstep = lflen / sflen;

	// number of samples per long audio frame
	filter->flen_long = lflen;

	// number of samples per short audio frame
	filter->flen_short = sflen;

	// input buffer (long frame)
	size = sizeof(float) * lflen;
	filter->in_long = (float *)fftwf_malloc(size);
	memset(filter->in_long, 0, size);

	// output buffer (long frame)
	size = sizeof(float) * lflen;
	filter->out_long = (float *)fftwf_malloc(size);
	memset(filter->out_long, 0, size);

	// convolution filter (short segments)
	size = sizeof(HConvSingle);
	filter->f_short = (HConvSingle *)malloc(size);
	hcInitSingle(filter->f_short, h, 2 * lflen, sflen, 1, fftOptimize);

	// convolution filter (long segments)
	size = sizeof(HConvSingle);
	filter->f_long = (HConvSingle *)malloc(size);
	hcInitSingle(filter->f_long, &(h[2 * lflen]), hlen - 2 * lflen, lflen, lflen / sflen, fftOptimize);

	if (h2 != NULL)
	{
		fftwf_free(h2);
	}
}


void hcCloseDual(HConvDual *filter)
{
	hcCloseSingle(filter->f_short);
	free(filter->f_short);
	hcCloseSingle(filter->f_long);
	free(filter->f_long);
	fftwf_free(filter->out_long);
	fftwf_free(filter->in_long);
	memset(filter, 0, sizeof(HConvDual));
}


void hcProcessTripple(HConvTripple *filter, float *in, float *out)
{
	int lpos, size, i;

	// convolution with short segments
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetSingle(filter->f_short, out);

	// add contribution from last medium frame
	lpos = filter->step * filter->flen_short;
	for (i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_medium[lpos + i];

	// add current frame to medium input buffer
	lpos = filter->step * filter->flen_short;
	size = sizeof(float) * filter->flen_short;
	memcpy(&(filter->in_medium[lpos]), in, size);

	// convolution with medium segments
	if (filter->step == filter->maxstep - 1)
		hcProcessDual(filter->f_medium,
		              filter->in_medium,
		              filter->out_medium);

	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcProcessAddTripple(HConvTripple *filter, float *in, float *out)
{
	int lpos, size, i;

	// convolution with short segments
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetAddSingle(filter->f_short, out);

	// add contribution from last medium frame
	lpos = filter->step * filter->flen_short;
	for (i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_medium[lpos + i];

	// add current frame to medium input buffer
	lpos = filter->step * filter->flen_short;
	size = sizeof(float) * filter->flen_short;
	memcpy(&(filter->in_medium[lpos]), in, size);

	// convolution with medium segments
	if (filter->step == filter->maxstep - 1)
		hcProcessDual(filter->f_medium,
		              filter->in_medium,
		              filter->out_medium);

	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcInitTripple(HConvTripple *filter, float *h, int hlen, int sflen, int mflen, int lflen, int fftOptimize)
{
	int size;
	float *h2 = NULL;
	int h2len;

	// sanity check: minimum impulse response length
	h2len = mflen + 2 * lflen + 1;
	if (hlen < h2len)
	{
		size = sizeof(float) * h2len;
		h2 = (float*)fftwf_malloc(size);
		memset(h2, 0, size);
		size = sizeof(float) * hlen;
		memcpy(h2, h, size);
		h = h2;
		hlen = h2len;
	}

	// processing step counter
	filter->step = 0;

	// number of processing steps per medium audio frame
	filter->maxstep = mflen / sflen;

	// number of samples per medium audio frame
	filter->flen_medium = mflen;

	// number of samples per short audio frame
	filter->flen_short = sflen;

	// input buffer (medium frame)
	size = sizeof(float) * mflen;
	filter->in_medium = (float *)fftwf_malloc(size);
	memset(filter->in_medium, 0, size);

	// output buffer (medium frame)
	size = sizeof(float) * mflen;
	filter->out_medium = (float *)fftwf_malloc(size);
	memset(filter->out_medium, 0, size);

	// convolution filter (short segments)
	size = sizeof(HConvSingle);
	filter->f_short = (HConvSingle *)malloc(size);
	hcInitSingle(filter->f_short, h, mflen, sflen, 1, fftOptimize);

	// convolution filter (medium segments)
	size = sizeof(HConvDual);
	filter->f_medium = (HConvDual *)malloc(size);
	hcInitDual(filter->f_medium, &(h[mflen]), hlen - mflen, mflen, lflen, fftOptimize);

	if (h2 != NULL)
	{
		fftwf_free(h2);
	}
}


void hcCloseTripple(HConvTripple *filter)
{
	hcCloseSingle(filter->f_short);
	free(filter->f_short);
	hcCloseDual(filter->f_medium);
	free(filter->f_medium);
	fftwf_free(filter->out_medium);
	fftwf_free(filter->in_medium);
	memset(filter, 0, sizeof(HConvTripple));
}
