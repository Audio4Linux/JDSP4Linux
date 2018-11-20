#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "kissfft/kiss_fftr.h"
#include "AutoConvolver.h"
typedef struct str_HConv1Stage
{
	int step;			// processing step counter
	int maxstep;			// number of processing steps per audio frame
	int mixpos;			// current frame index
	int framelength;		// number of samples per audio frame
	int *steptask;			// processing tasks per step
	double *dft_time;		// DFT buffer (time domain)
	kiss_fft_cpx *dft_freq;	// DFT buffer (frequency domain)
	double *in_freq_real;		// input buffer (frequency domain)
	double *in_freq_imag;		// input buffer (frequency domain)
	int num_filterbuf;		// number of filter segments
	double **filterbuf_freq_realChannel1;	// filter segments (frequency domain)
	double **filterbuf_freq_imagChannel1;	// filter segments (frequency domain)
	int num_mixbuf;			// number of mixing segments
	double **mixbuf_freq_real;	// mixing segments (frequency domain)
	double **mixbuf_freq_imag;	// mixing segments (frequency domain)
	double *history_time;		// history buffer (time domain)
	double normalizationGain;
	double gain;
	kiss_fftr_cfg fft;			// FFT transformation plan
	kiss_fftr_cfg ifft;		// IFFT transformation plan
	int memSize;
} HConv1Stage1x1;
inline void hcPut1Stage(HConv1Stage1x1 *filter, double *x)
{
	int j, flen, size;
	flen = filter->framelength;
	size = filter->memSize;
	memcpy(filter->dft_time, x, size);
	memset(&(filter->dft_time[flen]), 0, size);
	kiss_fftr(filter->fft, filter->dft_time, filter->dft_freq);
	for (j = 0; j < flen + 1; j++)
	{
		filter->in_freq_real[j] = filter->dft_freq[j].r;
		filter->in_freq_imag[j] = filter->dft_freq[j].i;
	}
}
void hcProcess1Stage(HConv1Stage1x1 *filter)
{
	int s, n, start, stop, flen;
	double *x_real;
	double *x_imag;
	double *h_real;
	double *h_imag;
	double *y_real;
	double *y_imag;
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
		h_real = filter->filterbuf_freq_realChannel1[s];
		h_imag = filter->filterbuf_freq_imagChannel1[s];
		for (n = 0; n < flen + 1; n++)
		{
			y_real[n] += (x_real[n] * h_real[n] - x_imag[n] * h_imag[n]);
			y_imag[n] += (x_real[n] * h_imag[n] + x_imag[n] * h_real[n]);
		}
	}
	filter->step = (filter->step + 1) % filter->maxstep;
}
inline void hcGet1Stage(HConv1Stage1x1 *filter, double *y)
{
	int flen, mpos;
	double *out;
	double *hist;
	int size, n, j;
	flen = filter->framelength;
	mpos = filter->mixpos;
	out = filter->dft_time;
	hist = filter->history_time;
	for (j = 0; j < flen + 1; j++)
	{
		filter->dft_freq[j].r = filter->mixbuf_freq_real[mpos][j];
		filter->dft_freq[j].i = filter->mixbuf_freq_imag[mpos][j];
		filter->mixbuf_freq_real[mpos][j] = 0.0f;
		filter->mixbuf_freq_imag[mpos][j] = 0.0f;
	}
	kiss_fftri(filter->ifft, filter->dft_freq, filter->dft_time);
	for (n = 0; n < flen; n++)
		y[n] = (out[n] + hist[n]) * filter->gain;
	size = filter->memSize;
	memcpy(hist, &(out[flen]), size);
	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}
void hcInit1Stage(HConv1Stage1x1 *filter, double *h, int hlen, int flen, int steps)
{
	int i, j, size, num, pos;
	// processing step counter
	filter->step = 0;
	// number of processing steps per audio frame
	filter->maxstep = steps;
	// current frame index
	filter->mixpos = 0;
	// number of samples per audio frame
	filter->framelength = flen;
	// DFT buffer (time domain)
	size = sizeof(double) * 2 * flen;
	filter->dft_time = (double *)malloc(size);
	// DFT buffer (frequency domain)
	size = sizeof(kiss_fft_cpx) * (flen + 1);
	filter->dft_freq = (kiss_fft_cpx*)malloc(size);
	// input buffer (frequency domain)
	size = sizeof(double) * (flen + 1);
	filter->in_freq_real = (double*)malloc(size);
	filter->in_freq_imag = (double*)malloc(size);
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
	size = sizeof(double*) * filter->num_filterbuf;
	filter->filterbuf_freq_realChannel1 = (double**)malloc(size);
	filter->filterbuf_freq_imagChannel1 = (double**)malloc(size);
	size = sizeof(double) * (flen + 1);
	for (i = 0; i < filter->num_filterbuf; i++)
	{
		filter->filterbuf_freq_realChannel1[i] = (double*)malloc(size);
		filter->filterbuf_freq_imagChannel1[i] = (double*)malloc(size);
	}
	// number of mixing segments
	filter->num_mixbuf = filter->num_filterbuf + 1;
	// mixing segments (frequency domain)
	size = sizeof(double*) * filter->num_mixbuf;
	filter->mixbuf_freq_real = (double**)malloc(size);
	filter->mixbuf_freq_imag = (double**)malloc(size);
	for (i = 0; i < filter->num_mixbuf; i++)
	{
		size = sizeof(double) * (flen + 1);
		filter->mixbuf_freq_real[i] = (double*)malloc(size);
		filter->mixbuf_freq_imag[i] = (double*)malloc(size);
		memset(filter->mixbuf_freq_real[i], 0, size);
		memset(filter->mixbuf_freq_imag[i], 0, size);
	}
	// history buffer (time domain)
	size = sizeof(double) * flen;
	filter->history_time = (double *)malloc(size);
	memset(filter->history_time, 0, size);
	// FFT transformation plan
	filter->fft = kiss_fftr_alloc(2 * flen, 0, 0, 0);
	// IFFT transformation plan
	filter->ifft = kiss_fftr_alloc(2 * flen, 1, 0, 0);
	// generate filter segments
	filter->normalizationGain = 0.5 / (double)flen;
	filter->gain = filter->normalizationGain;
	size = sizeof(double) * 2 * flen;
	memset(filter->dft_time, 0, size);
	for (i = 0; i < filter->num_filterbuf - 1; i++)
	{
		for (j = 0; j < flen; j++)
			filter->dft_time[j] = h[i * flen + j];
		kiss_fftr(filter->fft, filter->dft_time, filter->dft_freq);
		for (j = 0; j < flen + 1; j++)
		{
			filter->filterbuf_freq_realChannel1[i][j] = filter->dft_freq[j].r;
			filter->filterbuf_freq_imagChannel1[i][j] = filter->dft_freq[j].i;
		}
	}
	for (j = 0; j < hlen - i * flen; j++)
		filter->dft_time[j] = h[i * flen + j];
	size = sizeof(double) * ((i + 1) * flen - hlen);
	memset(&(filter->dft_time[hlen - i * flen]), 0, size);
	kiss_fftr(filter->fft, filter->dft_time, filter->dft_freq);
	for (j = 0; j < flen + 1; j++)
	{
		filter->filterbuf_freq_realChannel1[i][j] = filter->dft_freq[j].r;
		filter->filterbuf_freq_imagChannel1[i][j] = filter->dft_freq[j].i;
	}
	filter->memSize = sizeof(double) * flen;
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
void hcClose1Stage(HConv1Stage1x1 *filter)
{
	int i;
	free(filter->ifft);
	free(filter->fft);
	free(filter->history_time);
	for (i = 0; i < filter->num_mixbuf; i++)
	{
		free(filter->mixbuf_freq_real[i]);
		free(filter->mixbuf_freq_imag[i]);
	}
	free(filter->mixbuf_freq_real);
	free(filter->mixbuf_freq_imag);
	for (i = 0; i < filter->num_filterbuf; i++)
	{
		free(filter->filterbuf_freq_realChannel1[i]);
		free(filter->filterbuf_freq_imagChannel1[i]);
	}
	free(filter->filterbuf_freq_realChannel1);
	free(filter->filterbuf_freq_imagChannel1);
	free(filter->in_freq_real);
	free(filter->in_freq_imag);
	free(filter->dft_freq);
	free(filter->dft_time);
	free(filter->steptask);
	memset(filter, 0, sizeof(HConv1Stage1x1));
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
	HConv1Stage1x1 filter;
	double *x;
	double *h;
	double *y;
	int xlen, hlen, ylen;
	int size, n;
	int pos;
	double t_start, t_diff;
	double counter = 0.0;
	double proc_time;
	double lin, mul;

	xlen = 2048 * 2048;
	size = sizeof(double) * xlen;
	x = (double *)malloc(size);
	lin = pow(10.0, -100.0 / 20.0);	// 0.00001 = -100dB
	mul = pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (n = 1; n < xlen; n++)
		x[n] = (double)-mul * x[n - 1];

	hlen = flen * num;
	size = sizeof(double) * hlen;
	h = (double *)malloc(size);
	lin = pow(10.0, -60.0 / 20.0);	// 0.001 = -60dB
	mul = pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (n = 1; n < hlen; n++)
		h[n] = (double)mul * h[n - 1];

	ylen = flen;
	size = sizeof(double) * ylen;
	y = (double *)malloc(size);

	hcInit1Stage(&filter, h, hlen, flen, 1);

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
	free(x);
	free(h);
	free(y);
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
