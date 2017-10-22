#ifdef AUTOCONV_USE_SSE
#include <xmmintrin.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fftw3.h"
#include "AutoConvolver.h"
typedef struct str_dffirfilter
{
	unsigned int pos, coeffslength;
	float *coeffs, *delayLine;
} DFFIR;
typedef struct str_HConv1Stage
{
	int step;			// processing step counter
	int maxstep;			// number of processing steps per audio frame
	int mixpos;			// current frame index
	int framelength;		// number of samples per audio frame
	int *steptask;			// processing tasks per step
	float *dft_time;		// DFT buffer (time domain)
	fftwf_complex *dft_freq;	// DFT buffer (frequency domain)
	float *in_freq_real;		// input buffer (frequency domain)
	float *in_freq_imag;		// input buffer (frequency domain)
	int num_filterbuf;		// number of filter segments
	float **filterbuf_freq_real;	// filter segments (frequency domain)
	float **filterbuf_freq_imag;	// filter segments (frequency domain)
	int num_mixbuf;			// number of mixing segments
	float **mixbuf_freq_real;	// mixing segments (frequency domain)
	float **mixbuf_freq_imag;	// mixing segments (frequency domain)
	float *history_time;		// history buffer (time domain)
	fftwf_plan fft;			// FFT transformation plan
	fftwf_plan ifft;		// IFFT transformation plan
} HConv1Stage;
typedef struct str_HConv2Stage
{
	int step;		// processing step counter
	int maxstep;		// number of processing steps per long audio frame
	int flen_long;		// number of samples per long audio frame
	int flen_short;		// number of samples per short audio frame
	float *in_long;		// input buffer (long frame)
	float *out_long;	// output buffer (long frame)
	HConv1Stage *f_long;	// convolution filter (long segments)
	HConv1Stage *f_short;	// convolution filter (short segments)
} HConv2Stage;
typedef struct str_HConv3Stage
{
	int step;		// processing step counter
	int maxstep;		// number of processing steps per long audio frame
	int flen_medium;	// number of samples per long audio frame
	int flen_short;		// number of samples per short audio frame
	float *in_medium;	// input buffer (long frame)
	float *out_medium;	// output buffer (long frame)
	HConv2Stage *f_medium;	// convolution filter (long segments)
	HConv1Stage *f_short;	// convolution filter (short segments)
} HConv3Stage;
typedef struct str_HConv4Stage
{
	int step;		// processing step counter
	int maxstep;		// number of processing steps per long audio frame
	int flen_medium;	// number of samples per long audio frame
	int flen_short;		// number of samples per short audio frame
	float *in_medium;	// input buffer (long frame)
	float *out_medium;	// output buffer (long frame)
	HConv3Stage *f_medium;	// convolution filter (long segments)
	HConv1Stage *f_short;	// convolution filter (short segments)
} HConv4Stage;
int ACFFTWThreadInit()
{
	return fftwf_init_threads();
}
void ACFFTWClean(int threads)
{
	if (threads)
		fftwf_cleanup_threads();
	else
		fftwf_cleanup();
}
void DFFIRInit(DFFIR *fir, float *h, int hlen)
{
	int i, size;
	fir->pos = 0;
	fir->coeffslength = hlen;
	size = hlen * sizeof(float);
	fir->coeffs = (float*)malloc(size);
	for (i = 0; i < hlen; i++)
		fir->coeffs[i] = h[i];
	fir->delayLine = (float*)malloc(size);
	memset(fir->delayLine, 0, size);
}
void DFFIRClean(DFFIR *fir)
{
	free(fir->coeffs);
	free(fir->delayLine);
	memset(fir, 0, sizeof(DFFIR));
}
float DFFIRProcess(DFFIR *fir, float x)
{
	float *coeff = fir->coeffs;
	float *coeff_end = fir->coeffs + fir->coeffslength;
	float *buf_val = fir->delayLine + fir->pos;
	*buf_val = x;
	float y = 0.0f;
	while (buf_val >= fir->delayLine)
		y += *buf_val-- * *coeff++;
	buf_val = fir->delayLine + fir->coeffslength - 1;
	while (coeff < coeff_end)
		y += *buf_val-- * *coeff++;
	if (++fir->pos >= fir->coeffslength)
		fir->pos = 0;
	return y;
}
inline void hcPut1Stage(HConv1Stage *filter, float *x)
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
void hcProcess1Stage(HConv1Stage *filter)
{
#ifdef AUTOCONV_USE_SSE
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
		y_real[flen] += x_real[flen] * h_real[flen] - x_imag[flen] * h_imag[flen];
		y_imag[flen] += x_real[flen] * h_imag[flen] + x_imag[flen] * h_real[flen];
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
			y_real[n] += x_real[n] * h_real[n] - x_imag[n] * h_imag[n];
			y_imag[n] += x_real[n] * h_imag[n] + x_imag[n] * h_real[n];
		}
	}
	filter->step = (filter->step + 1) % filter->maxstep;
#endif
}
inline void hcGet1Stage(HConv1Stage *filter, float *y)
{
	int flen, mpos;
	float *out;
	float *hist;
	int size, n, j;
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
	{
		y[n] = out[n] + hist[n];
	}
	size = sizeof(float) * flen;
	memcpy(hist, &(out[flen]), size);
	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}
void hcInit1Stage(HConv1Stage *filter, float *h, int hlen, int flen, int steps, int fftwThreads)
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
	if (fftwThreads)
		fftwf_plan_with_nthreads(fftwThreads);
	// FFT transformation plan
	filter->fft = fftwf_plan_dft_r2c_1d(2 * flen, filter->dft_time, filter->dft_freq, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
	// IFFT transformation plan
	filter->ifft = fftwf_plan_dft_c2r_1d(2 * flen, filter->dft_freq, filter->dft_time, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
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
void hcProcess2Stage(HConv2Stage *filter, float *in, float *out)
{
	int lpos, size, i;
	// convolution with short segments
	hcPut1Stage(filter->f_short, in);
	hcProcess1Stage(filter->f_short);
	hcGet1Stage(filter->f_short, out);
	// add contribution from last long frame
	lpos = filter->step * filter->flen_short;
	for (i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_long[lpos + i];
	// convolution with long segments
	if (filter->step == 0)
		hcPut1Stage(filter->f_long, filter->in_long);
	hcProcess1Stage(filter->f_long);
	if (filter->step == filter->maxstep - 1)
		hcGet1Stage(filter->f_long, filter->out_long);
	// add current frame to long input buffer
	lpos = filter->step * filter->flen_short;
	size = sizeof(float) * filter->flen_short;
	memcpy(&(filter->in_long[lpos]), in, size);
	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}
void hcInit2Stage(HConv2Stage *filter, float *h, int hlen, int sflen, int lflen, int fftwThreads)
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
	size = sizeof(HConv1Stage);
	filter->f_short = (HConv1Stage *)malloc(size);
	hcInit1Stage(filter->f_short, h, 2 * lflen, sflen, 1, fftwThreads);
	// convolution filter (long segments)
	size = sizeof(HConv1Stage);
	filter->f_long = (HConv1Stage *)malloc(size);
	hcInit1Stage(filter->f_long, &(h[2 * lflen]), hlen - 2 * lflen, lflen, lflen / sflen, fftwThreads);
	if (h2 != NULL)
		fftwf_free(h2);
}
void hcProcess3Stage(HConv3Stage *filter, float *in, float *out)
{
	int lpos, size, i;
	// convolution with short segments
	hcPut1Stage(filter->f_short, in);
	hcProcess1Stage(filter->f_short);
	hcGet1Stage(filter->f_short, out);
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
		hcProcess2Stage(filter->f_medium, filter->in_medium, filter->out_medium);
	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}
void hcInit3Stage(HConv3Stage *filter, float *h, int hlen, int sflen, int mflen, int lflen, int fftwThreads)
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
	size = sizeof(HConv1Stage);
	filter->f_short = (HConv1Stage *)malloc(size);
	hcInit1Stage(filter->f_short, h, mflen, sflen, 1, fftwThreads);
	// convolution filter (medium segments)
	size = sizeof(HConv2Stage);
	filter->f_medium = (HConv2Stage *)malloc(size);
	hcInit2Stage(filter->f_medium, &(h[mflen]), hlen - mflen, mflen, lflen, fftwThreads);
	if (h2 != NULL)
		fftwf_free(h2);
}
void hcProcess4Stage(HConv4Stage *filter, float *in, float *out)
{
	int lpos, size, i;
	// convolution with short segments
	hcPut1Stage(filter->f_short, in);
	hcProcess1Stage(filter->f_short);
	hcGet1Stage(filter->f_short, out);
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
		hcProcess3Stage(filter->f_medium, filter->in_medium, filter->out_medium);
	// increase step counter
	filter->step = (filter->step + 1) % filter->maxstep;
}
void hcInit4Stage(HConv4Stage *filter, float *h, int hlen, int ssflen, int sflen, int mflen, int lflen, int fftwThreads)
{
	int size;
	float *h2 = NULL;
	int h2len;
	// sanity check: minimum impulse response length
	h2len = sflen + mflen + 2 * lflen + 1;
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
	filter->maxstep = sflen / ssflen;
	// number of samples per medium audio frame
	filter->flen_medium = sflen;
	// number of samples per short audio frame
	filter->flen_short = ssflen;
	// input buffer (medium frame)
	size = sizeof(float) * sflen;
	filter->in_medium = (float *)fftwf_malloc(size);
	memset(filter->in_medium, 0, size);
	// output buffer (medium frame)
	size = sizeof(float) * sflen;
	filter->out_medium = (float *)fftwf_malloc(size);
	memset(filter->out_medium, 0, size);
	// convolution filter (short segments)
	size = sizeof(HConv1Stage);
	filter->f_short = (HConv1Stage *)malloc(size);
	hcInit1Stage(filter->f_short, h, sflen, ssflen, 1, fftwThreads);
	// convolution filter (medium segments)
	size = sizeof(HConv3Stage);
	filter->f_medium = (HConv3Stage *)malloc(size);
	hcInit3Stage(filter->f_medium, &(h[sflen]), hlen - sflen, sflen, mflen, lflen, fftwThreads);
	if (h2 != NULL)
		fftwf_free(h2);
}
void Convolver4StageProcessArbitrarySignalLength(AutoConvolver *autoConv, float** inputs, float** outputs, int sigLen)
{
	int pos, new_pos;
	int nChannels = autoConv->channels;
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv4Stage *m_filter = (HConv4Stage*)autoConv->filter;
	for (int f = 0; f < nChannels; f++)
	{
		pos = autoConv->bufpos;
		float *in = inputs[f];
		float *out = outputs[f];
		float *buf_in = &(m_inbuf[f * m_lenShort]);
		float *buf_out = &(m_outbuf[f * m_lenShort]);
		for (int s = 0; s < sigLen; s++)
		{
			buf_in[pos] = in[s];
			out[s] = buf_out[pos];
			pos++;
			if (pos == m_lenShort)
			{
				hcProcess4Stage(&(m_filter[f]), buf_in, buf_out);
				pos = 0;
			}
		}
		new_pos = pos;
	}
	autoConv->bufpos = new_pos;
}
void Convolver3StageProcessArbitrarySignalLength(AutoConvolver *autoConv, float** inputs, float** outputs, int sigLen)
{
	int pos, new_pos;
	int nChannels = autoConv->channels;
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv3Stage *m_filter = (HConv3Stage*)autoConv->filter;
	for (int f = 0; f < nChannels; f++)
	{
		pos = autoConv->bufpos;
		float *in = inputs[f];
		float *out = outputs[f];
		float *buf_in = &(m_inbuf[f * m_lenShort]);
		float *buf_out = &(m_outbuf[f * m_lenShort]);
		for (int s = 0; s < sigLen; s++)
		{
			buf_in[pos] = in[s];
			out[s] = buf_out[pos];
			pos++;
			if (pos == m_lenShort)
			{
				hcProcess3Stage(&(m_filter[f]), buf_in, buf_out);
				pos = 0;
			}
		}
		new_pos = pos;
	}
	autoConv->bufpos = new_pos;
}
void Convolver2StageProcessArbitrarySignalLength(AutoConvolver *autoConv, float** inputs, float** outputs, int sigLen)
{
	int pos, new_pos;
	int nChannels = autoConv->channels;
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv2Stage *m_filter = (HConv2Stage*)autoConv->filter;
	for (int f = 0; f < nChannels; f++)
	{
		pos = autoConv->bufpos;
		float *in = inputs[f];
		float *out = outputs[f];
		float *buf_in = &(m_inbuf[f * m_lenShort]);
		float *buf_out = &(m_outbuf[f * m_lenShort]);
		for (int s = 0; s < sigLen; s++)
		{
			buf_in[pos] = in[s];
			out[s] = buf_out[pos];
			pos++;
			if (pos == m_lenShort)
			{
				hcProcess2Stage(&(m_filter[f]), buf_in, buf_out);
				pos = 0;
			}
		}
		new_pos = pos;
	}
	autoConv->bufpos = new_pos;
}
void Convolver1StageLowLatencyProcess(AutoConvolver *autoConv, float** inputs, float** outputs, int unused)
{
	int nChannels = autoConv->channels;
	HConv1Stage *m_filter = (HConv1Stage*)autoConv->filter;
	for (int f = 0; f < nChannels; f++)
	{
		float *in = inputs[f];
		float *out = outputs[f];
		hcPut1Stage(&(m_filter[f]), in);
		hcProcess1Stage(&(m_filter[f]));
		hcGet1Stage(&(m_filter[f]), out);
	}
}
void Convolver1DirectFormProcess(AutoConvolver *autoConv, float** inputs, float** outputs, int sigLen)
{
	int i, nChannels = autoConv->channels;
	DFFIR *m_filter = (DFFIR*)autoConv->filter;
	for (int f = 0; f < nChannels; f++)
	{
		float *in = inputs[f];
		float *out = outputs[f];
		for (i = 0; i < sigLen; i++)
			out[i] = DFFIRProcess(&(m_filter[f]), in[i]);
	}
}
void Convolver4StageProcessArbitrarySignalLengthMono(AutoConvolverMono *autoConv, float* inputs, float* outputs, int sigLen)
{
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv4Stage *m_filter = (HConv4Stage*)autoConv->filter;
	int pos = autoConv->bufpos;
	for (int s = 0; s < sigLen; s++)
	{
		m_inbuf[pos] = inputs[s];
		outputs[s] = m_outbuf[pos];
		pos++;
		if (pos == m_lenShort)
		{
			hcProcess4Stage(m_filter, m_inbuf, m_outbuf);
			pos = 0;
		}
	}
	autoConv->bufpos = pos;
}
void Convolver3StageProcessArbitrarySignalLengthMono(AutoConvolverMono *autoConv, float* inputs, float* outputs, int sigLen)
{
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv3Stage *m_filter = (HConv3Stage*)autoConv->filter;
	int pos = autoConv->bufpos;
	for (int s = 0; s < sigLen; s++)
	{
		m_inbuf[pos] = inputs[s];
		outputs[s] = m_outbuf[pos];
		pos++;
		if (pos == m_lenShort)
		{
			hcProcess3Stage(m_filter, m_inbuf, m_outbuf);
			pos = 0;
		}
	}
	autoConv->bufpos = pos;
}
void Convolver2StageProcessArbitrarySignalLengthMono(AutoConvolverMono *autoConv, float* inputs, float* outputs, int sigLen)
{
	int m_lenShort = autoConv->hnShortLen;
	float *m_inbuf = autoConv->inbuf;
	float *m_outbuf = autoConv->outbuf;
	HConv2Stage *m_filter = (HConv2Stage*)autoConv->filter;
	int pos = autoConv->bufpos;
	for (int s = 0; s < sigLen; s++)
	{
		m_inbuf[pos] = inputs[s];
		outputs[s] = m_outbuf[pos];
		pos++;
		if (pos == m_lenShort)
		{
			hcProcess2Stage(m_filter, m_inbuf, m_outbuf);
			pos = 0;
		}
	}
	autoConv->bufpos = pos;
}
void Convolver1StageLowLatencyProcessMono(AutoConvolverMono *autoConv, float* inputs, float* outputs, int unused)
{
	HConv1Stage *m_filter = (HConv1Stage*)autoConv->filter;
	hcPut1Stage(m_filter, inputs);
	hcProcess1Stage(m_filter);
	hcGet1Stage(m_filter, outputs);
}
void Convolver1DirectFormProcessMono(AutoConvolverMono *autoConv, float* inputs, float* outputs, int sigLen)
{
	DFFIR *m_filter = (DFFIR*)autoConv->filter;
	for (int i = 0; i < sigLen; i++)
		outputs[i] = DFFIRProcess(m_filter, inputs[i]);
}
int PartitionerAnalyser(int hlen, int latency, int strategy, int fs, int entriesResult, double **result_c0_c1, int *sflen_best, int *mflen_best, int *lflen_best)
{
	if (hlen < 0)
		return 0;
	else if (hlen < 32)
		return 999;
	int s, m, l, begin_m, end_m, type_best, num_s, num_m, num_l, sflen, mflen, lflen;
	double cpu_load, tau_s, tau_m, tau_l;
	double *c0 = result_c0_c1[0];
	double *c1 = result_c0_c1[1];
	if (latency < 128)
		latency = 128;
	s = (int)log2(latency) - 6;
	if (!strategy)
	{
		begin_m = 2;
		end_m = 3;
	}
	else
	{
		begin_m = 1;
		end_m = entriesResult - s - 1;
	}
	sflen = latency;
	double cpu_load_best = 1e12;
	// performance prediction with 3 segment lengths
	for (m = begin_m; m < end_m; m++)
	{
		for (l = 1; l + m + s < entriesResult; l++)
		{
			mflen = sflen << m;
			lflen = mflen << l;
			num_s = mflen / sflen;
			num_m = 2 * lflen / mflen;
			num_l = (int)(ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen));
			if (num_l < 1)
				num_l = 1;
			tau_s = c0[s] + c1[s] * num_s;
			tau_m = c0[s + m] + c1[s + m] * num_m;
			tau_l = c0[s + m + l] + c1[s + m + l] * num_l;
			cpu_load = 400.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * fs / (double)lflen;
			if (cpu_load < cpu_load_best)
			{
				cpu_load_best = cpu_load;
				*sflen_best = sflen;
				*mflen_best = mflen;
				*lflen_best = lflen;
				type_best = 3;
			}
		}
	}
	// performance prediction with 2 segment lengths
	begin_m = 1;
	end_m = entriesResult - s;
	for (m = begin_m; m < end_m; m++)
	{
		mflen = sflen << m;
		num_s = 2 * mflen / sflen;
		num_m = (int)(ceil((hlen - num_s * sflen) / (double)mflen));
		if (num_m < 1)
			num_m = 1;
		tau_s = c0[s] + c1[s] * num_s;
		tau_m = c0[s + m] + c1[s + m] * num_m;
		cpu_load = 400.0 * (tau_s * mflen / sflen + tau_m) * fs / (double)mflen;
		if (cpu_load < cpu_load_best)
		{
			cpu_load_best = cpu_load;
			*sflen_best = sflen;
			*mflen_best = mflen;
			type_best = 2;
		}
	}
	// performance prediction with 1 segment length
	num_s = (int)(ceil(hlen / (double)sflen));
	tau_s = c0[s] + c1[s] * num_s;
	cpu_load = 400.0 * tau_s * fs / (double)sflen;
	if (cpu_load < cpu_load_best)
	{
		cpu_load_best = cpu_load;
		*sflen_best = hlen;
		type_best = 1;
	}
	return type_best;
}
AutoConvolver* InitAutoConvolver(float **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, double **recommendation, int items, int fftwThreads)
{
	int i, bestMethod = 1, sflen_best = 4096, mflen_best = 8192, lflen_best = 16384, llflen_best = 32768;
	hlen = abs(hlen);
	if (!hlen)
		return 0;
	if (recommendation)
		bestMethod = PartitionerAnalyser(hlen, 4096, 1, 48000, items, recommendation, &sflen_best, &mflen_best, &lflen_best);
	if (hlen > 0 && hlen < 32)
		bestMethod = 999;
	else if (hlen > 20000 && hlen < 81921 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 2048;
		mflen_best = 8192;
	}
	else if (hlen > 81920 && hlen < 245764 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 4096;
		mflen_best = 8192;
	}
	else if (hlen > 245763 && hlen < 1000001 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 4096;
		mflen_best = 16384;
	}
	else if (hlen > 1000000 && hlen < 2000001 && bestMethod < 2)
		bestMethod = 3;
	else if (hlen > 2000000 && bestMethod < 2)
		bestMethod = 4;
	AutoConvolver *autoConv = (AutoConvolver*)calloc(1, sizeof(AutoConvolver));
	autoConv->channels = nChannels;
	autoConv->methods = bestMethod;
	if (bestMethod > 1)
	{
		autoConv->hnShortLen = sflen_best;
		autoConv->inbuf = (float*)calloc(nChannels * sflen_best, sizeof(float));
		autoConv->outbuf = (float*)calloc(nChannels * sflen_best, sizeof(float));
	}
	if (bestMethod == 3)
	{
		HConv3Stage* stage = (HConv3Stage*)malloc(nChannels * sizeof(HConv3Stage));
		for (i = 0; i < nChannels; i++)
			hcInit3Stage(&stage[i], impulseResponse[(i % impChannels)], hlen, sflen_best, mflen_best, lflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver3StageProcessArbitrarySignalLength;
	}
	else if (bestMethod == 2)
	{
		HConv2Stage* stage = (HConv2Stage*)malloc(nChannels * sizeof(HConv2Stage));
		for (i = 0; i < nChannels; i++)
			hcInit2Stage(&(stage[i]), impulseResponse[i % impChannels], hlen, sflen_best, mflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver2StageProcessArbitrarySignalLength;
	}
	else if (bestMethod == 4)
	{
		HConv4Stage* stage = (HConv4Stage*)malloc(nChannels * sizeof(HConv4Stage));
		for (i = 0; i < nChannels; i++)
			hcInit4Stage(&(stage[i]), impulseResponse[i % impChannels], hlen, sflen_best, mflen_best, lflen_best, llflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver4StageProcessArbitrarySignalLength;
	}
	else if (bestMethod == 999)
	{
		DFFIR* stage = (DFFIR*)malloc(nChannels * sizeof(DFFIR));
		for (i = 0; i < nChannels; i++)
			DFFIRInit(&stage[i], impulseResponse[i % impChannels], hlen);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver1DirectFormProcess;
	}
	else
	{
		HConv1Stage* stage = (HConv1Stage*)malloc(nChannels * sizeof(HConv1Stage));
		for (i = 0; i < nChannels; i++)
			hcInit1Stage(&(stage[i]), impulseResponse[i % impChannels], hlen, audioBufferSize, 1, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver1StageLowLatencyProcess;
	}
	return autoConv;
}
AutoConvolverMono* InitAutoConvolverMono(float *impulseResponse, int hlen, int audioBufferSize, double **recommendation, int items, int fftwThreads)
{
	int bestMethod = 1, sflen_best = 4096, mflen_best = 8192, lflen_best = 16384, llflen_best = 32768;
	hlen = abs(hlen);
	if (!hlen)
		return 0;
	if (recommendation)
		bestMethod = PartitionerAnalyser(hlen, 4096, 1, 48000, items, recommendation, &sflen_best, &mflen_best, &lflen_best);
	if (hlen > 0 && hlen < 32)
		bestMethod = 999;
	else if (hlen > 20000 && hlen < 81921 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 2048;
		mflen_best = 8192;
	}
	else if (hlen > 81920 && hlen < 245764 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 4096;
		mflen_best = 8192;
	}
	else if (hlen > 245763 && hlen < 1000001 && bestMethod < 2)
	{
		bestMethod = 2;
		sflen_best = 4096;
		mflen_best = 16384;
	}
	else if (hlen > 1000000 && hlen < 2000001 && bestMethod < 2)
		bestMethod = 3;
	else if (hlen > 2000000 && bestMethod < 2)
		bestMethod = 4;
	AutoConvolverMono *autoConv = (AutoConvolverMono*)calloc(1, sizeof(AutoConvolverMono));
	autoConv->methods = bestMethod;
	if (bestMethod > 1)
	{
		autoConv->hnShortLen = sflen_best;
		autoConv->inbuf = (float*)calloc(sflen_best, sizeof(float));
		autoConv->outbuf = (float*)calloc(sflen_best, sizeof(float));
	}
	if (bestMethod == 3)
	{
		HConv3Stage* stage = (HConv3Stage*)malloc(sizeof(HConv3Stage));
		hcInit3Stage(stage, impulseResponse, hlen, sflen_best, mflen_best, lflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver3StageProcessArbitrarySignalLengthMono;
	}
	else if (bestMethod == 2)
	{
		HConv2Stage* stage = (HConv2Stage*)malloc(sizeof(HConv2Stage));
		hcInit2Stage(stage, impulseResponse, hlen, sflen_best, mflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver2StageProcessArbitrarySignalLengthMono;
	}
	else if (bestMethod == 4)
	{
		HConv4Stage* stage = (HConv4Stage*)malloc(sizeof(HConv4Stage));
		hcInit4Stage(stage, impulseResponse, hlen, sflen_best, mflen_best, lflen_best, llflen_best, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver4StageProcessArbitrarySignalLengthMono;
	}
	else if (bestMethod == 999)
	{
		DFFIR* stage = (DFFIR*)malloc(sizeof(DFFIR));
		DFFIRInit(stage, impulseResponse, hlen);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver1DirectFormProcessMono;
	}
	else
	{
		HConv1Stage* stage = (HConv1Stage*)malloc(sizeof(HConv1Stage));
		hcInit1Stage(stage, impulseResponse, hlen, audioBufferSize, 1, fftwThreads);
		autoConv->filter = (void*)stage;
		autoConv->process = &Convolver1StageLowLatencyProcessMono;
	}
	return autoConv;
}
AutoConvolver* InitAutoConvolverZeroLatency(float **impulseResponse, int hlen, int impChannels, int audioBufferSize, int nChannels, int fftwThreads)
{
	AutoConvolver *autoConv = (AutoConvolver*)calloc(1, sizeof(AutoConvolver));
	autoConv->channels = nChannels;
	autoConv->methods = 1;
	HConv1Stage* stage = (HConv1Stage*)malloc(nChannels * sizeof(HConv1Stage));
	for (int i = 0; i < nChannels; i++)
		hcInit1Stage(&(stage[i]), impulseResponse[i % impChannels], hlen, audioBufferSize, 1, fftwThreads);
	autoConv->filter = (void*)stage;
	autoConv->process = &Convolver1StageLowLatencyProcess;
	return autoConv;
}
AutoConvolverMono* InitAutoConvolverMonoZeroLatency(float *impulseResponse, int hlen, int audioBufferSize, int fftwThreads)
{
	AutoConvolverMono *autoConv = (AutoConvolverMono*)calloc(1, sizeof(AutoConvolverMono));
	autoConv->methods = 1;
	HConv1Stage* stage = (HConv1Stage*)malloc(sizeof(HConv1Stage));
	hcInit1Stage(stage, impulseResponse, hlen, audioBufferSize, 1, fftwThreads);
	autoConv->filter = (void*)stage;
	autoConv->process = &Convolver1StageLowLatencyProcessMono;
	return autoConv;
}
void hcClose1Stage(HConv1Stage *filter)
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
	memset(filter, 0, sizeof(HConv1Stage));
}
void hcClose2Stage(HConv2Stage *filter)
{
	hcClose1Stage(filter->f_short);
	free(filter->f_short);
	hcClose1Stage(filter->f_long);
	free(filter->f_long);
	fftwf_free(filter->out_long);
	fftwf_free(filter->in_long);
	memset(filter, 0, sizeof(HConv2Stage));
}
void hcClose3Stage(HConv3Stage *filter)
{
	hcClose1Stage(filter->f_short);
	free(filter->f_short);
	hcClose2Stage(filter->f_medium);
	free(filter->f_medium);
	fftwf_free(filter->out_medium);
	fftwf_free(filter->in_medium);
	memset(filter, 0, sizeof(HConv3Stage));
}
void hcClose4Stage(HConv4Stage *filter)
{
	hcClose1Stage(filter->f_short);
	free(filter->f_short);
	hcClose3Stage(filter->f_medium);
	free(filter->f_medium);
	fftwf_free(filter->out_medium);
	fftwf_free(filter->in_medium);
	memset(filter, 0, sizeof(HConv4Stage));
}
void AutoConvolverFree(AutoConvolver *autoConv)
{
	int i, nChannels = autoConv->channels;
	if (autoConv->methods > 1)
	{
		free(autoConv->inbuf);
		free(autoConv->outbuf);
	}
	if (autoConv->methods == 1)
	{
		HConv1Stage* stage = (HConv1Stage*)autoConv->filter;
		for (i = 0; i < nChannels; i++)
			hcClose1Stage(&stage[i]);
		free(stage);
	}
	else if (autoConv->methods == 2)
	{
		HConv2Stage* stage = (HConv2Stage*)autoConv->filter;
		for (i = 0; i < nChannels; i++)
			hcClose2Stage(&stage[i]);
		free(stage);
	}
	else if (autoConv->methods == 3)
	{
		HConv3Stage* stage = (HConv3Stage*)autoConv->filter;
		for (i = 0; i < nChannels; i++)
			hcClose3Stage(&stage[i]);
		free(stage);
	}
	else if (autoConv->methods == 4)
	{
		HConv4Stage* stage = (HConv4Stage*)autoConv->filter;
		for (i = 0; i < nChannels; i++)
			hcClose4Stage(&stage[i]);
		free(stage);
	}
	else if (autoConv->methods == 999)
	{
		DFFIR* stage = (DFFIR*)autoConv->filter;
		for (i = 0; i < nChannels; i++)
			DFFIRClean(&stage[i]);
		free(stage);
	}
}
void AutoConvolverMonoFree(AutoConvolverMono *autoConv)
{
	if (autoConv->methods > 1)
	{
		free(autoConv->inbuf);
		free(autoConv->outbuf);
	}
	if (autoConv->methods == 1)
	{
		HConv1Stage* stage = (HConv1Stage*)autoConv->filter;
		hcClose1Stage(stage);
		free(stage);
	}
	else if (autoConv->methods == 2)
	{
		HConv2Stage* stage = (HConv2Stage*)autoConv->filter;
		hcClose2Stage(stage);
		free(stage);
	}
	else if (autoConv->methods == 3)
	{
		HConv3Stage* stage = (HConv3Stage*)autoConv->filter;
		hcClose3Stage(stage);
		free(stage);
	}
	else if (autoConv->methods == 4)
	{
		HConv4Stage* stage = (HConv4Stage*)autoConv->filter;
		hcClose4Stage(stage);
		free(stage);
	}
	else if (autoConv->methods == 999)
	{
		DFFIR* stage = (DFFIR*)autoConv->filter;
		DFFIRClean(stage);
		free(stage);
	}
}
#ifdef _WIN32
#include <Windows.h>
double hcTime(void)
{
	unsigned long long t;
	t = GetTickCount();
	return (double)t * 0.001;
}
#else
#include <sys/time.h>
double hcTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 0.000001;
}
#endif
double getProcTime(int flen, int num, double dur)
{
	HConv1Stage filter;
	float *x;
	float *h;
	float *y;
	int xlen, hlen, ylen;
	int size, n;
	int pos;
	double t_start, t_diff;
	double counter = 0.0;
	double proc_time;
	double lin, mul;

	xlen = 2048 * 2048;
	size = sizeof(float) * xlen;
	x = (float *)fftwf_malloc(size);
	lin = pow(10.0, -100.0 / 20.0);	// 0.00001 = -100dB
	mul = pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (n = 1; n < xlen; n++)
		x[n] = (float)-mul * x[n - 1];

	hlen = flen * num;
	size = sizeof(float) * hlen;
	h = (float *)fftwf_malloc(size);
	lin = pow(10.0, -60.0 / 20.0);	// 0.001 = -60dB
	mul = pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (n = 1; n < hlen; n++)
		h[n] = (float)mul * h[n - 1];

	ylen = flen;
	size = sizeof(float) * ylen;
	y = (float *)fftwf_malloc(size);

	hcInit1Stage(&filter, h, hlen, flen, 1, 0);

	t_diff = 0.0;
	t_start = hcTime();
	pos = 0;
	while (t_diff < dur)
	{
		hcPut1Stage(&filter, &x[pos]);
		hcProcess1Stage(&filter);
		hcGet1Stage(&filter, y);
		pos += flen;
		if (pos >= xlen)
			pos = 0;
		counter += 1.0;
		t_diff = hcTime() - t_start;
	}
	proc_time = t_diff / counter;
	printf("Processing time: %7.3f us\n", 1000000.0 * proc_time);
	hcClose1Stage(&filter);
	fftwf_free(x);
	fftwf_free(h);
	fftwf_free(y);
	return proc_time;
}
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet)
{
	FILE *wisdom_file = fopen("partition_wisdom.txt", "r");
	if (!wisdom_file)
		return 0;
	int items, size;
	fscanf(wisdom_file, "Items: %d\n", &items);
	if (items > 15)
		items = 15;
	double **result_c0_c1 = (double**)malloc(2 * sizeof(double*));
	result_c0_c1[0] = (double*)malloc(items * sizeof(double));
	result_c0_c1[1] = (double*)malloc(items * sizeof(double));
	double tmp_c0 = 1.0, tmp_c1 = 1.0;
	int i = 0;
	while (EOF != fscanf(wisdom_file, "%d %lf, %lf\n", &size, &tmp_c0, &tmp_c1))
	{
		if (i < items)
		{
			result_c0_c1[0][i] = tmp_c0;
			result_c0_c1[1][i] = tmp_c1;
		}
		i++;
	}
	fclose(wisdom_file);
	*itemRet = items;
	return result_c0_c1;
}
#ifdef _WIN32
/* For some reason, MSVC fails to honour this #ifndef. */
/* Hence function renamed to _vscprintf_so(). */
int _vscprintf_so(const char * format, va_list pargs)
{
	int retval;
	va_list argcopy;
	va_copy(argcopy, pargs);
	retval = vsnprintf(NULL, 0, format, argcopy);
	va_end(argcopy);
	return retval;
}
int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int len = _vscprintf_so(fmt, ap);
	if (len == -1) return -1;
	char *str = malloc((size_t)len + 1);
	if (!str) return -1;
	int r = vsnprintf(str, len + 1, fmt, ap); /* "secure" version of vsprintf */
	if (r == -1) return free(str), -1;
	*strp = str;
	return r;
}
int asprintf(char *strp[], const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}
#endif
int str_append(char **bufchar, const char *format, ...)
{
	char *str = NULL;
	char *old_bufchar = NULL, *new_bufchar = NULL;
	va_list arg_ptr;
	va_start(arg_ptr, format);
	vasprintf(&str, format, arg_ptr);
	asprintf(&old_bufchar, "%s", (*bufchar == NULL ? "" : *bufchar));
	new_bufchar = (char *)calloc(strlen(old_bufchar) + strlen(str) + 1, sizeof(char));
	if (!new_bufchar)
		return 0;
	strcat(new_bufchar, old_bufchar);
	strcat(new_bufchar, str);
	if (*bufchar) free(*bufchar);
	*bufchar = new_bufchar;
	free(old_bufchar);
	free(str);
	return 1;
}
char* PartitionHelper(int s_max, int fs)
{
	const int sflen_start = 256;
	const int hlen = 2 * fs;

	int s, m, l;
	int sflen, mflen, lflen;
	int sflen_best = 256,
		mflen_best = 1024,
		lflen_best = 4096;
	int num;
	double tau_1, tau_16;
	double cpu_load, cpu_load_best;
	double *c0 = (double*)malloc(s_max * sizeof(double));
	double *c1 = (double*)malloc(s_max * sizeof(double));
	double tau_s, tau_m, tau_l;
	int num_s, num_m, num_l;
	int counter = 0;
#ifndef __ANDROID__
	puts("Measurement Results:");
#endif
	// performance measurement with uniform segmentation
	for (s = 0; s < s_max; s++)
	{
		sflen = sflen_start << s;
		num = 1;
		tau_1 = getProcTime(sflen, num, 0.001);	// avoid FFTW wisdom measurement in our measurement
		tau_1 = getProcTime(sflen, num, 1.0);
		counter++;
#ifndef __ANDROID__
		printf("Processing time for %d segment of length %d:  %5.3f us\n", num, sflen, 1000000.0 * tau_1);
#endif
		num = 16;
		tau_16 = getProcTime(sflen, num, 0.001);	// avoid FFTW wisdom measurement in our measurement
		tau_16 = getProcTime(sflen, num, 1.0);
		counter++;
#ifndef __ANDROID__
		printf("Processing time for %d segments of length %d:  %5.3f us\n", num, sflen, 1000000.0 * tau_16);
#endif
		c1[s] = (tau_16 - tau_1) / 15.0;
		c0[s] = tau_1 - c1[s];
#ifndef __ANDROID__
		printf("Processing time for a FFT of length %d: \t%5.3f us\n", sflen, 1000000.0 * c0[s]);
		printf("Processing time for a block multiplication of length %d:  %5.3f us\n", sflen, 1000000.0 * c1[s]);
	}
	printf("\n\nPredicted CPU load for 4 FIR filters of 2.0s length at %d Hz sampling rate:\n", fs);
#else
}
#endif
	// performance prediction with 3 segment lengths
	for (s = 0; s < 5; s++)
	{
		// uniform processing load
		cpu_load_best = 1e12;
		m = 2;
		for (l = 1; l + m + s < s_max; l++)
		{
			sflen = 256 << s;
			mflen = sflen << m;
			lflen = mflen << l;

			num_s = mflen / sflen;
			num_m = 2 * lflen / mflen;
			num_l = (int)(ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen));
			if (num_l < 0)
				num_l = 1;
			tau_s = c0[s] + c1[s] * num_s;
			tau_m = c0[s + m] + c1[s + m] * num_m;
			tau_l = c0[s + m + l] + c1[s + m + l] * num_l;

			cpu_load = 4.0 * 100.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * fs / (double)lflen;
			if (cpu_load < cpu_load_best)
			{
				cpu_load_best = cpu_load;
				sflen_best = sflen;
				mflen_best = mflen;
				lflen_best = lflen;
			}
		}
#ifndef __ANDROID__
		printf("%d samples latency, uniform CPU load (%d/%d/%d): %5.2f %%\n", sflen_best, sflen_best, mflen_best, lflen_best, cpu_load_best);
#endif
		// lowest mean processing load
		cpu_load_best = 1e12;
		for (m = 1; m < 5; m++)
		{
			for (l = 1; l + m + s < s_max; l++)
			{
				sflen = 256 << s;
				mflen = sflen << m;
				lflen = mflen << l;

				num_s = mflen / sflen;
				num_m = 2 * lflen / mflen;
				num_l = (int)(ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen));
				if (num_l < 0)
					num_l = 1;
				tau_s = c0[s] + c1[s] * num_s;
				tau_m = c0[s + m] + c1[s + m] * num_m;
				tau_l = c0[s + m + l] + c1[s + m + l] * num_l;

				cpu_load = 4.0 * 100.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * fs / (double)lflen;
				if (cpu_load < cpu_load_best)
				{
					cpu_load_best = cpu_load;
					sflen_best = sflen;
					mflen_best = mflen;
					lflen_best = lflen;
				}
			}
		}
#ifndef __ANDROID__
		printf("%d samples latency, lowest mean load (%d/%d/%d): %5.2f %%\n", sflen_best, sflen_best, mflen_best, lflen_best, cpu_load_best);
#endif
	}
	char *wisdom_str = NULL;
	str_append(&wisdom_str, "Items: %d\n", s_max);
	for (s = 0; s < s_max; s++)
	{
		sflen = sflen_start << s;
		str_append(&wisdom_str, "%d %14.15f, %14.15f\n", sflen, c0[s], c1[s]);
	}
	free(c0);
	free(c1);
	return wisdom_str;
}
double** PartitionHelperDirect(int s_max, int fs)
{
	const int sflen_start = 256;
	const int hlen = 2 * fs;

	int s, m, l;
	int sflen, mflen, lflen;
	int sflen_best = 256,
		mflen_best = 1024,
		lflen_best = 4096;
	int num;
	double tau_1, tau_16;
	double cpu_load, cpu_load_best;
	double *c0 = (double*)malloc(s_max * sizeof(double));
	double *c1 = (double*)malloc(s_max * sizeof(double));
	double tau_s, tau_m, tau_l;
	int num_s, num_m, num_l;
	int counter = 0;
#ifndef __ANDROID__
	puts("Measurement Results:");
#endif
	// performance measurement with uniform segmentation
	for (s = 0; s < s_max; s++)
	{
		sflen = sflen_start << s;
		num = 1;
		tau_1 = getProcTime(sflen, num, 0.001);	// avoid FFTW wisdom measurement in our measurement
		tau_1 = getProcTime(sflen, num, 1.0);
		counter++;
#ifndef __ANDROID__
		printf("Processing time for %d segment of length %d:  %5.3f us\n", num, sflen, 1000000.0 * tau_1);
#endif
		num = 16;
		tau_16 = getProcTime(sflen, num, 0.001);	// avoid FFTW wisdom measurement in our measurement
		tau_16 = getProcTime(sflen, num, 1.0);
		counter++;
#ifndef __ANDROID__
		printf("Processing time for %d segments of length %d:  %5.3f us\n", num, sflen, 1000000.0 * tau_16);
#endif
		c1[s] = (tau_16 - tau_1) / 15.0;
		c0[s] = tau_1 - c1[s];
#ifndef __ANDROID__
		printf("Processing time for a FFT of length %d: \t%5.3f us\n", sflen, 1000000.0 * c0[s]);
		printf("Processing time for a block multiplication of length %d:  %5.3f us\n", sflen, 1000000.0 * c1[s]);
	}
	printf("\n\nPredicted CPU load for 4 FIR filters of 2.0s length at %d Hz sampling rate:\n", fs);
#else
}
#endif
	// performance prediction with 3 segment lengths
	for (s = 0; s < 5; s++)
	{
		// uniform processing load
		cpu_load_best = 1e12;
		m = 2;
		for (l = 1; l + m + s < s_max; l++)
		{
			sflen = 256 << s;
			mflen = sflen << m;
			lflen = mflen << l;

			num_s = mflen / sflen;
			num_m = 2 * lflen / mflen;
			num_l = (int)(ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen));
			if (num_l < 0)
				num_l = 1;
			tau_s = c0[s] + c1[s] * num_s;
			tau_m = c0[s + m] + c1[s + m] * num_m;
			tau_l = c0[s + m + l] + c1[s + m + l] * num_l;

			cpu_load = 4.0 * 100.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * fs / (double)lflen;
			if (cpu_load < cpu_load_best)
			{
				cpu_load_best = cpu_load;
				sflen_best = sflen;
				mflen_best = mflen;
				lflen_best = lflen;
			}
		}
#ifndef __ANDROID__
		printf("%d samples latency, uniform CPU load (%d/%d/%d): %5.2f %%\n", sflen_best, sflen_best, mflen_best, lflen_best, cpu_load_best);
#endif
		// lowest mean processing load
		cpu_load_best = 1e12;
		for (m = 1; m < 5; m++)
		{
			for (l = 1; l + m + s < s_max; l++)
			{
				sflen = 256 << s;
				mflen = sflen << m;
				lflen = mflen << l;

				num_s = mflen / sflen;
				num_m = 2 * lflen / mflen;
				num_l = (int)(ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen));
				if (num_l < 0)
					num_l = 1;
				tau_s = c0[s] + c1[s] * num_s;
				tau_m = c0[s + m] + c1[s + m] * num_m;
				tau_l = c0[s + m + l] + c1[s + m + l] * num_l;

				cpu_load = 4.0 * 100.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * fs / (double)lflen;
				if (cpu_load < cpu_load_best)
				{
					cpu_load_best = cpu_load;
					sflen_best = sflen;
					mflen_best = mflen;
					lflen_best = lflen;
				}
			}
		}
#ifndef __ANDROID__
		printf("%d samples latency, lowest mean load (%d/%d/%d): %5.2f %%\n", sflen_best, sflen_best, mflen_best, lflen_best, cpu_load_best);
#endif
	}
	double **retBenchmark = (double**)malloc(2 * sizeof(double*));
	retBenchmark[0] = c0;
	retBenchmark[1] = c1;
	return retBenchmark;
}