#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include "Effects/eel2/dr_flac.h"
#include "Effects/eel2/ns-eel.h"
#include "jdsp_header.h"
#define TAG "EffectDSPMain"

#ifdef __ANDROID_API__
#if __ANDROID_API__ < __ANDROID_API_J_MR2__
double log2(double x) {
    return (log(x)*1.4426950408889634);
}
#endif
#endif

// Range: 0x800000, 0x7fffff
int32_t i32_from_p24_big_endian(const uint8_t *packed24)
{
	return (packed24[2] << 8) | (packed24[1] << 16) | (packed24[0] << 24);
}
int32_t i32_from_p24_little_endian(const uint8_t *packed24)
{
	return (packed24[0] << 8) | (packed24[1] << 16) | (packed24[2] << 24);
}
void p24_from_i32_big_endian(int32_t ival, uint8_t *packed24)
{
	uint8_t *dst = packed24;
	*dst++ = ival >> 16;
	*dst++ = ival >> 8;
	*dst++ = ival;
}
void p24_from_i32_little_endian(int32_t ival, uint8_t *packed24)
{
	uint8_t *dst = packed24;
	*dst++ = ival;
	*dst++ = ival >> 8;
	*dst++ = ival >> 16;
}
int32_t clamp24_from_float(float f)
{
	static const float scale = (float)(1 << 23);
	float limpos = 0x7fffff / scale;
	float limneg = -0x800000 / scale;
	if (f <= limneg)
		return -0x800000;
	else if (f >= limpos)
		return 0x7fffff;
	f *= scale;
	/* integer conversion is through truncation (though int to float is not).
	 * ensure that we round to nearest, ties away from 0.
	 */
	return f > 0 ? f + 0.5f : f - 0.5f;
}
double crossPlatformCurTime(void)
{
#ifdef WIN32
	DWORD t;

	t = GetTickCount();
	return (double)t * 0.001;
#else
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 0.000001;
#endif
}
double getProcTime(int flen, int num, double dur)
{
	int pos;
	double t_start, t_diff;
	int counter = 0.0;
	double proc_time;

	int xlen = 2048 * 2048;
	float *x = (float *)malloc(sizeof(float) * xlen);
	float lin = (float)pow(10.0, -100.0 / 20.0);	// 0.00001 = -100dB
	float mul = (float)pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (int n = 1; n < xlen; n++)
		x[n] = -mul * x[n - 1];

	int hlen = flen * num;
	float *h = (float *)malloc(sizeof(float) * hlen);
	lin = (float)pow(10.0, -60.0 / 20.0);	// 0.001 = -60dB
	mul = (float)pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (int n = 1; n < hlen; n++)
		h[n] = mul * h[n - 1];

	float *y = (float *)malloc(sizeof(float) * flen);

	FFTConvolver1x1 filter;
	FFTConvolver1x1Init(&filter);
	FFTConvolver1x1LoadImpulseResponse(&filter, flen, h, hlen);

	t_diff = 0.0;
	t_start = crossPlatformCurTime();
	pos = 0;
	while (t_diff < dur)
	{
		FFTConvolver1x1Process(&filter, &x[pos], y, flen);
		pos += flen;
		if (pos >= xlen)
			pos = 0;
		counter++;
		t_diff = crossPlatformCurTime() - t_start;
	}
	FFTConvolver1x1Free(&filter);
	free(x);
	free(h);
	free(y);
	return t_diff / (double)counter;
}
char benchmarkEnable = 0;
char benchmarkCompletionFlag = 0;
double convbench_c0[MAX_BENCHMARK] = { 2.4999999e-05f,4.9999999e-05f,9.9999997e-05f,0.0002f,0.0005f,0.001f,0.0021f,0.0057f,0.0126f,0.0293f };
double convbench_c1[MAX_BENCHMARK] = { 3.1249999e-06f,6.2499998e-06f,1.2500000e-05f,2.4999999e-05f,4.9999999e-05f,9.9999997e-05f,0.0002f,0.0004f,0.0009f,0.0019f };
void JamesDSP_Load_benchmark(double *_c0, double *_c1)
{
	memcpy(convbench_c0, _c0, sizeof(convbench_c0));
	memcpy(convbench_c1, _c1, sizeof(convbench_c1));
	benchmarkCompletionFlag = 1;
	benchmarkEnable = 1;
}
void JamesDSP_Save_benchmark(double *_c0, double *_c1)
{
	memcpy(_c0, convbench_c0, sizeof(convbench_c0));
	memcpy(_c1, convbench_c1, sizeof(convbench_c1));
}
void JamesDSP_Start_benchmark()
{
    benchmarkEnable = 1;

	const int sflen_start = 64;
	int s, m;
	int sflen_best = sflen_start;
	int mflen_best = 256;
	int counter = 0;
	double _c0[MAX_BENCHMARK];
	double _c1[MAX_BENCHMARK];
	for (s = 0; s < MAX_BENCHMARK; s++)
	{
		int sflen = sflen_start << s;
		int num = 1;
		double tau_1 = getProcTime(sflen, num, 1.0);
		counter++;
		num = 16;
		double tau_16 = getProcTime(sflen, num, 1.0);
		counter++;
		_c1[s] = (tau_16 - tau_1) / 15.0;
		_c0[s] = tau_1 - convbench_c1[s];
	}
	JamesDSP_Load_benchmark(_c0, _c1);
#ifdef DEBUG
	__android_log_print(ANDROID_LOG_INFO, TAG, "Benchmark done");
#endif
}
void *convBench(void *arg)
{
	size_t sleepMs = 30000; // 30 second after library being loaded
#ifdef _WIN32
	Sleep(sleepMs);
#else
	usleep(sleepMs * 1000ULL);
#endif
#ifdef DEBUG
	__android_log_print(ANDROID_LOG_INFO, TAG, "Benchmark start");
#endif
	JamesDSP_Start_benchmark();
	pthread_exit(NULL);
	return 0;
}
int selectConvPartitions(JamesDSPLib *jdsp, unsigned int impulseLengthActual, unsigned int *seg2Len)
{
	double tau_s, tau_m;
	int mflen_best = 256;
	int num_s, num_m;
	int latency = jdsp->blockSize;
	int type_best, begin_m, end_m;
	int s = (int)log2(latency) - 6;
	if (s < 0)
	{
		printf("Latency must not smaller than 64\n");
		return 1;
	}
	double cpu_load;
	double cpu_load_best = 1e12;
	// performance prediction with 2 segment lengths
	begin_m = 1;
	end_m = MAX_BENCHMARK - s;
	for (int m = begin_m; m < end_m; m++)
	{
		int mflen = latency << m;
		num_s = 2 * mflen / latency;
		num_m = (int)ceil((impulseLengthActual - num_s * latency) / (double)mflen);
		if (num_m < 1)
			num_m = 1;
		tau_s = convbench_c0[s] + convbench_c1[s] * num_s;
		tau_m = convbench_c0[s + m] + convbench_c1[s + m] * num_m;
		cpu_load = 400.0 * (tau_s * mflen / latency + tau_m) * jdsp->fs / (double)mflen;
		if (cpu_load < cpu_load_best)
		{
			cpu_load_best = cpu_load;
			mflen_best = mflen;
			type_best = 2;
		}
	}
	// performance prediction with 1 segment length
	num_s = (int)ceil(impulseLengthActual / (double)latency);
	tau_s = convbench_c0[s] + convbench_c1[s] * num_s;
	cpu_load = 400.0 * tau_s * jdsp->fs / (double)latency;
	if (cpu_load < cpu_load_best)
	{
		cpu_load_best = cpu_load;
		type_best = 1;
	}
	*seg2Len = mflen_best;
	return type_best;
}
void JamesDSPGlobalMemoryAllocation()
{
	benchmarkCompletionFlag = 0;
	NSEEL_start();
#ifdef JAMESDSP_REFERENCE_IMPL
	pthread_t benchmarkThread;
	pthread_create(&benchmarkThread, NULL, convBench, 0);
#endif
}
void JamesDSPGlobalMemoryDeallocation()
{
	NSEEL_quit();
}
unsigned int next_pow_2(unsigned int x)
{
	if (x <= 1) return 1;
	int power = 2;
	x--;
	while (x >>= 1) power <<= 1;
	return power;
}
void JamesDSPReallocateBlock(JamesDSPLib *jdsp, size_t n)
{
	// Init buffer
	float *tmp1 = jdsp->tmpBuffer[0];
	float *tmp2 = jdsp->tmpBuffer[1];
	float *tmp3 = jdsp->tmpBuffer[2];
	float *tmp4 = jdsp->tmpBuffer[3];
	float *tmp5 = jdsp->tmpBuffer[4];
	float *tmp6 = jdsp->tmpBuffer[5];
	double ratio = (double)jdsp->fs / (double)jdsp->trueSampleRate;
	jdsp->blockSizeMax = n;
	unsigned int maxDecimatedLength = (unsigned int)ceil(jdsp->blockSizeMax * ratio);
	if (!jdsp->enableASRC)
		maxDecimatedLength = 0;
	unsigned int maxInterpolatedLength = (unsigned int)ceil(maxDecimatedLength / ratio);
	jdsp->pw2BlockMemSize = next_pow_2(maxInterpolatedLength);
	if (!jdsp->enableASRC)
		jdsp->pw2BlockMemSize = 0;
	size_t ctMemBlk = jdsp->blockSizeMax * 2 + maxInterpolatedLength * 2 + jdsp->pw2BlockMemSize * 2;
	jdsp->tmpBuffer[0] = (float *)malloc(ctMemBlk * sizeof(float));
	jdsp->tmpBuffer[1] = jdsp->tmpBuffer[0] + jdsp->blockSizeMax;
	jdsp->tmpBuffer[2] = jdsp->tmpBuffer[1] + jdsp->blockSizeMax;
	jdsp->tmpBuffer[3] = jdsp->tmpBuffer[2] + maxInterpolatedLength;
	jdsp->tmpBuffer[4] = jdsp->tmpBuffer[3] + maxInterpolatedLength;
	jdsp->tmpBuffer[5] = jdsp->tmpBuffer[4] + jdsp->pw2BlockMemSize;
	if (tmp1)
		free(tmp1);
}
void JamesDSPRefreshConvolutions(JamesDSPLib *jdsp, char refreshAll)
{
    // Temporary(?) bug fix when benchmarks are off
   if(!benchmarkEnable) {
#ifdef DEBUG
        __android_log_print(ANDROID_LOG_INFO, TAG, "ignoring call to JamesDSPRefreshConvolutions(...) because benchmarks are disabled");
#endif
        return;
    }

#ifdef DEBUG
	__android_log_print(ANDROID_LOG_INFO, TAG, "Buffer size changed, update convolution object to maximize performance");
#endif
	if (jdsp->impulseResponseStorage.impulseResponse)
		Convolver1DLoadImpulseResponse(jdsp, jdsp->impulseResponseStorage.impulseResponse, jdsp->impulseResponseStorage.impChannels, jdsp->impulseResponseStorage.impulseLengthActual, 0);
	jdsp->crossfeedForceRefresh = 1;
	CrossfeedEnable(jdsp, jdsp->crossfeedEnabled);
	if (refreshAll)
	{
		jdsp->arbMagForceRefresh = 1;
		ArbitraryResponseEqualizerEnable(jdsp, jdsp->arbitraryMagEnabled);
		jdsp->equalizerForceRefresh = 1;
		MultimodalEqualizerEnable(jdsp, jdsp->equalizerEnabled);
	}
}
void jdsp_lock(JamesDSPLib *jdsp)
{
	if (jdsp->isMutexSuccess)
		pthread_mutex_lock(&jdsp->m_in_processing);
}
void jdsp_unlock(JamesDSPLib *jdsp)
{
	if (jdsp->isMutexSuccess)
		pthread_mutex_unlock(&jdsp->m_in_processing);
}
// Process
void JamesDSPProcess(JamesDSPLib *jdsp, size_t n)
{
	// Analog modelling
	if (jdsp->tubeEnabled)
		VacuumTubeProcess(jdsp, n);
	// Input / Compressor
	if (jdsp->compEnabled)
		CompressorProcess(jdsp, n);
	// IIR bass boost
	if (jdsp->bassBoostEnabled)
		BassBoostProcess(jdsp, n);
	// Equalizer
	if (jdsp->equalizerEnabled)
		MultimodalEqualizerProcess(jdsp, n);
	// Arbitrary magnitude eq
	if (jdsp->arbitraryMagEnabled)
		ArbitraryResponseEqualizerProcess(jdsp, n);
	jdsp_lock(jdsp);
	// Convolver
	if (jdsp->convolverEnabled)
		if (jdsp->conv.process)
			jdsp->conv.process(jdsp, n);
	// Viper DDC
	if (jdsp->ddcEnabled)
		DDCProcess(jdsp, n);
	// Live programmable
	if (jdsp->liveprogEnabled)
		LiveProgProcess(jdsp, n);
	jdsp_unlock(jdsp);
	// BS2B
	if (jdsp->crossfeedEnabled)
		CrossfeedProcess(jdsp, n);
	// Stereo widening
	if (jdsp->sterEnhEnabled)
		StereoEnhancementProcess(jdsp, n);
	// Reverb
	if (jdsp->reverbEnabled)
		ReverbProcess(jdsp, n);
	// Output
	for (size_t i = 0; i < n; i++)
	{
		float xL = jdsp->tmpBuffer[0][i] * jdsp->postGain;
		float xR = jdsp->tmpBuffer[1][i] * jdsp->postGain;
		float rect1 = fabsf(xL);
		float rect2 = fabsf(xR);
		float maxLR = max(rect1, rect2);
		if (maxLR < jdsp->limiter.threshold)
			maxLR = jdsp->limiter.threshold;
		if (maxLR > jdsp->limiter.envOverThreshold)
			jdsp->limiter.envOverThreshold = maxLR;
		else
			jdsp->limiter.envOverThreshold = maxLR + jdsp->limiter.relCoef * (jdsp->limiter.envOverThreshold - maxLR);
		float gR = jdsp->limiter.threshold / jdsp->limiter.envOverThreshold;
		rect1 = xL * gR;
		rect2 = xR * gR;
		jdsp->tmpBuffer[0][i] = rect1;
		jdsp->tmpBuffer[1][i] = rect2;
	}
}
void JamesDSPProcessCheckBenchmarkReady(JamesDSPLib *jdsp, size_t n)
{
	// Analog modelling
	if (jdsp->tubeEnabled)
		VacuumTubeProcess(jdsp, n);
	// Input / Compressor
	if (jdsp->compEnabled)
		CompressorProcess(jdsp, n);
	// IIR bass boost
	if (jdsp->bassBoostEnabled)
		BassBoostProcess(jdsp, n);
	// Equalizer
	if (jdsp->equalizerEnabled)
		MultimodalEqualizerProcess(jdsp, n);
	// Arbitrary magnitude eq
	if (jdsp->arbitraryMagEnabled)
		ArbitraryResponseEqualizerProcess(jdsp, n);
	jdsp_lock(jdsp);
	// Convolver
	if (jdsp->convolverEnabled)
		if (jdsp->conv.process)
			jdsp->conv.process(jdsp, n);
	// Viper DDC
	if (jdsp->ddcEnabled)
		DDCProcess(jdsp, n);
	// Live programmable
	if (jdsp->liveprogEnabled)
		LiveProgProcess(jdsp, n);
	jdsp_unlock(jdsp);
	// BS2B
	if (jdsp->crossfeedEnabled)
		CrossfeedProcess(jdsp, n);
	// Stereo widening
	if (jdsp->sterEnhEnabled)
		StereoEnhancementProcess(jdsp, n);
	// Reverb
	if (jdsp->reverbEnabled)
		ReverbProcess(jdsp, n);
	// Output
	for (size_t i = 0; i < n; i++)
	{
		float xL = jdsp->tmpBuffer[0][i] * jdsp->postGain;
		float xR = jdsp->tmpBuffer[1][i] * jdsp->postGain;
		float rect1 = fabsf(xL);
		float rect2 = fabsf(xR);
		float maxLR = max(rect1, rect2);
		if (maxLR < jdsp->limiter.threshold)
			maxLR = jdsp->limiter.threshold;
		if (maxLR > jdsp->limiter.envOverThreshold)
			jdsp->limiter.envOverThreshold = maxLR;
		else
			jdsp->limiter.envOverThreshold = maxLR + jdsp->limiter.relCoef * (jdsp->limiter.envOverThreshold - maxLR);
		float gR = jdsp->limiter.threshold / jdsp->limiter.envOverThreshold;
		rect1 = xL * gR;
		rect2 = xR * gR;
		jdsp->tmpBuffer[0][i] = rect1;
		jdsp->tmpBuffer[1][i] = rect2;
	}
	if (benchmarkCompletionFlag == 1)
	{
#ifdef DEBUG
		__android_log_print(ANDROID_LOG_INFO, TAG, "Benchmark flag == 1, refreshing convolutions");
#endif
		JamesDSPRefreshConvolutions(jdsp, 0);
		jdsp->processInternal = JamesDSPProcess;
	}
}
size_t iabs(size_t value)
{
	return value < 0 ? 0 - value : value;
}
void sample_ratio(unsigned long long numerator, unsigned long long denominator, unsigned long long *num, unsigned long long *denom)
{
	unsigned long long largest = numerator > denominator ? numerator : denominator;
	unsigned long long gcd = 0; // greatest common divisor
	for (unsigned long long i = 2; i <= largest; i++) // initializes i to 2, if i is less that or equal to the largest, then increment the i counter.
		if (numerator % i == 0 && denominator % i == 0) //if the i's remainder equals 0 and the denominator's remainder equals 0,
			gcd = i; // then the greatest common denominator is assigned the value of the i.
	if (gcd != 0) // if the greatest common denominator does not equal 0, the divide both the numerator and the denominator by the greatest common denominator.
	{
		*num = numerator / gcd;
		*denom = denominator / gcd;
	}
	else
	{
		*num = numerator;
		*denom = denominator;
	}
}
void RingBuffer_Init(RingBuffer *fifo)
{
	fifo->in = fifo->out = 0;
}
uint32_t RingBuffering(RingBuffer *fifo, const float *buffer, const float *in, uint32_t lenIn, float *out, uint32_t lenOut, uint32_t size)
{
	lenIn = min(lenIn, size - (fifo->in - fifo->out));
	// First put the data starting from fifo->in to buffer end
	uint32_t l_In = min(lenIn, size - (fifo->in & (size - 1)));
	memcpy(buffer + (fifo->in & (size - 1)), in, l_In * sizeof(float));
	// Then put the rest (if any) at the beginning of the buffer
	memcpy(buffer, in + l_In, (lenIn - l_In) * sizeof(float));
	fifo->in += lenIn;
	lenOut = min(lenOut, fifo->in - fifo->out);
	unsigned int lenOut_2 = min(lenOut, fifo->in - fifo->out);
	// First get the data from fifo->out until the end of the buffer
	uint32_t l_Out = min(lenOut, size - (fifo->out & (size - 1)));
	memcpy(out, buffer + (fifo->out & (size - 1)), l_Out * sizeof(float));
	// Then get the rest (if any) from the beginning of the buffer
	memcpy(out + l_Out, buffer, (lenOut - l_Out) * sizeof(float));
	fifo->out += lenOut;
	if (fifo->in > size || fifo->out > size)
	{
		fifo->in = fifo->in - size;
		fifo->out = fifo->out - size;
	}
	return lenOut;
}
uint32_t RingBufferingStereo(RingBuffer *fifo[2], const float *buffer1, const float *buffer2, const float *in1, const float *in2, uint32_t lenIn, float *out1, float *out2, uint32_t lenOut, uint32_t size)
{
	lenIn = min(lenIn, size - (fifo[0]->in - fifo[0]->out));
	// First put the data starting from fifo[0]->in to buffer end
	uint32_t l_In = min(lenIn, size - (fifo[0]->in & (size - 1)));
	memcpy(buffer1 + (fifo[0]->in & (size - 1)), in1, l_In * sizeof(float));
	memcpy(buffer2 + (fifo[0]->in & (size - 1)), in2, l_In * sizeof(float));
	// Then put the rest (if any) at the beginning of the buffer
	memcpy(buffer1, in1 + l_In, (lenIn - l_In) * sizeof(float));
	memcpy(buffer2, in2 + l_In, (lenIn - l_In) * sizeof(float));
	fifo[0]->in += lenIn;
	lenOut = min(lenOut, fifo[0]->in - fifo[0]->out);
	unsigned int lenOut_2 = min(lenOut, fifo[0]->in - fifo[0]->out);
	// First get the data from fifo[0]->out until the end of the buffer
	uint32_t l_Out = min(lenOut, size - (fifo[0]->out & (size - 1)));
	memcpy(out1, buffer1 + (fifo[0]->out & (size - 1)), l_Out * sizeof(float));
	memcpy(out2, buffer2 + (fifo[0]->out & (size - 1)), l_Out * sizeof(float));
	// Then get the rest (if any) from the beginning of the buffer
	memcpy(out1 + l_Out, buffer1, (lenOut - l_Out) * sizeof(float));
	memcpy(out2 + l_Out, buffer2, (lenOut - l_Out) * sizeof(float));
	fifo[0]->out += lenOut;
	if (fifo[0]->in > size || fifo[0]->out > size)
	{
		fifo[0]->in = fifo[0]->in - size;
		fifo[0]->out = fifo[0]->out - size;
	}
	return lenOut;
}
void InitIntegerASRCHandler(IntegerASRCHandler *asrc, unsigned long long workingFs, unsigned long long inFs, unsigned int polyphaseFIRTaps, char minphase, SRCResampler *inst1, SRCResampler *inst2)
{
	double ratio = (double)workingFs / (double)inFs;
	unsigned int minimumInputBufLen = (unsigned int)ceil((double)inFs / (double)workingFs);
	asrc->calculatedLatencyWholeSystem = minimumInputBufLen;
	unsigned long long num, denom;
	sample_ratio(workingFs, inFs, &num, &denom);
	if ((workingFs == num && inFs == denom) || num > 2000)
		minphase = 0;
	unsigned int maxDecimatedLength = (unsigned int)ceil(asrc->calculatedLatencyWholeSystem * ratio);
	if (!inst1)
		psrc_generate(&asrc->polyphaseDecimator, num, denom, polyphaseFIRTaps, 0.99, minphase);
	else
		psrc_clone(&asrc->polyphaseDecimator, inst1);
	if (!inst2)
		psrc_generate(&asrc->polyphaseInterpolator, denom, num, polyphaseFIRTaps, 0.99, minphase);
	else
		psrc_clone(&asrc->polyphaseInterpolator, inst2);
	//
	unsigned int maxInterpolatedLength = (unsigned int)ceil(maxDecimatedLength / ratio);
	RingBuffer_Init(&asrc->intermediateRing);
}
void FreeIntegerASRCHandler(IntegerASRCHandler *asrc)
{
	psrc_free(&asrc->polyphaseDecimator);
	psrc_free(&asrc->polyphaseInterpolator);
}
unsigned int DoASRC_fwd(JamesDSPLib *jdsp, size_t n)
{
	//unsigned int curDecimatedLen = psrc_filt(&jdsp->asrc[0].polyphaseDecimator, jdsp->tmpBuffer[0], n, jdsp->tmpBuffer[2]);
	//curDecimatedLen = psrc_filt(&jdsp->asrc[1].polyphaseDecimator, jdsp->tmpBuffer[1], n, jdsp->tmpBuffer[3]);
	SRCResampler *ptr[2] = { &jdsp->asrc[0].polyphaseDecimator, &jdsp->asrc[1].polyphaseDecimator };
	unsigned int curDecimatedLen = psrc_filt_stereo(ptr, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], n, jdsp->tmpBuffer[2], jdsp->tmpBuffer[3]);
	for (unsigned int i = 0; i < curDecimatedLen; i++)
	{
		jdsp->tmpBuffer[0][i] = jdsp->tmpBuffer[2][i];
		jdsp->tmpBuffer[1][i] = jdsp->tmpBuffer[3][i];
	}
	return curDecimatedLen;
}
void DoASRC_bwd(JamesDSPLib *jdsp, unsigned int curDecimatedLen, size_t n)
{
	//unsigned int curInterpolatedLen = psrc_filt(&jdsp->asrc[0].polyphaseInterpolator, jdsp->tmpBuffer[0], curDecimatedLen, jdsp->tmpBuffer[2]);
	//curInterpolatedLen = psrc_filt(&jdsp->asrc[1].polyphaseInterpolator, jdsp->tmpBuffer[1], curDecimatedLen, jdsp->tmpBuffer[3]);
	SRCResampler *ptr[2] = { &jdsp->asrc[0].polyphaseInterpolator, &jdsp->asrc[1].polyphaseInterpolator };
	unsigned int curInterpolatedLen = psrc_filt_stereo(ptr, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], curDecimatedLen, jdsp->tmpBuffer[2], jdsp->tmpBuffer[3]);
	//unsigned int dequeued = RingBuffering(&jdsp->asrc[0].intermediateRing, jdsp->tmpBuffer[4], jdsp->tmpBuffer[2], curInterpolatedLen, jdsp->tmpBuffer[0], n, jdsp->pw2BlockMemSize);
	//dequeued = RingBuffering(&jdsp->asrc[1].intermediateRing, jdsp->tmpBuffer[5], jdsp->tmpBuffer[3], curInterpolatedLen, jdsp->tmpBuffer[1], n, jdsp->pw2BlockMemSize);
	RingBuffer *ptr2[2] = { &jdsp->asrc[0].intermediateRing, &jdsp->asrc[1].intermediateRing };
	unsigned int dequeued = RingBufferingStereo(ptr2, jdsp->tmpBuffer[4], jdsp->tmpBuffer[5], jdsp->tmpBuffer[2], jdsp->tmpBuffer[3], curInterpolatedLen, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], n, jdsp->pw2BlockMemSize);
	const int howManyItemsLeft1 = (int)jdsp->asrc[0].intermediateRing.in - (int)jdsp->asrc[0].intermediateRing.out;
	const int howManyItemsLeft2 = (int)jdsp->asrc[0].intermediateRing.in - (int)jdsp->asrc[0].intermediateRing.out;
}
void pint16(JamesDSPLib *jdsp, int16_t *x1, int16_t *x2, int16_t *y1, int16_t *y2, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = (float)(1UL << 15UL);
	static const float offset = (float)(3 << (22 - 15));
	/* zero = (0x10f << 22) =  0x43c00000 (not directly used) */
	static const int32_t limneg = (0x10f << 22) /*zero*/ - 32768; /* 0x43bf8000 */
	static const int32_t limpos = (0x10f << 22) /*zero*/ + 32767; /* 0x43c07fff */
	union {
		float f;
		int32_t i;
	} u;
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = x1[i] / scale;
		jdsp->tmpBuffer[1][i] = x2[i] / scale;
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		u.f = jdsp->tmpBuffer[0][i] + offset;
		if (u.i < limneg)
			u.i = -32768;
		else if (u.i > limpos)
			u.i = 32767;
		y1[i] = u.i;
		u.f = jdsp->tmpBuffer[1][i] + offset;
		if (u.i < limneg)
			u.i = -32768;
		else if (u.i > limpos)
			u.i = 32767;
		y2[i] = u.i;
	}
}
void pint16Multiplexed(JamesDSPLib *jdsp, int16_t *x, int16_t *y, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float offset = (float)(3 << (22 - 15));
	/* zero = (0x10f << 22) =  0x43c00000 (not directly used) */
	static const int32_t limneg = (0x10f << 22) /*zero*/ - 32768; /* 0x43bf8000 */
	static const int32_t limpos = (0x10f << 22) /*zero*/ + 32767; /* 0x43c07fff */
	union {
		float f;
		int32_t i;
	} u;
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = x[i << 1] * 0.000030517578125f;
		jdsp->tmpBuffer[1][i] = x[(i << 1) + 1] * 0.000030517578125f;
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		u.f = jdsp->tmpBuffer[0][i] + offset;
		if (u.i < limneg)
			u.i = -32768;
		else if (u.i > limpos)
			u.i = 32767;
		y[i << 1] = (int16_t)u.i;
		u.f = jdsp->tmpBuffer[1][i] + offset;
		if (u.i < limneg)
			u.i = -32768;
		else if (u.i > limpos)
			u.i = 32767;
		y[(i << 1) + 1] = (int16_t)u.i;
	}
}
void pint32(JamesDSPLib *jdsp, int32_t *x1, int32_t *x2, int32_t *y1, int32_t *y2, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = (float)(1UL << 31UL);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = (float)((double)x1[i] / scale);
		jdsp->tmpBuffer[1][i] = (float)((double)x2[i] / scale);
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	float f;
	for (size_t i = 0; i < n; i++)
	{
		if (jdsp->tmpBuffer[0][i] <= -1.0f)
			y1[i] = INT32_MIN;
		else if (jdsp->tmpBuffer[0][i] >= 1.0f)
			y1[i] = INT32_MAX;
		else
		{
			f = jdsp->tmpBuffer[0][i] * scale;
			y1[i] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		}
		if (jdsp->tmpBuffer[1][i] <= -1.0f)
			y2[i] = INT32_MIN;
		else if (jdsp->tmpBuffer[1][i] >= 1.0f)
			y2[i] = INT32_MAX;
		else
		{
			f = jdsp->tmpBuffer[1][i] * scale;
			y2[i] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		}
	}
}
void pint32Multiplexed(JamesDSPLib *jdsp, int32_t *x, int32_t *y, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = (float)(1UL << 31UL);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = (float)((double)x[i << 1] / scale);
		jdsp->tmpBuffer[1][i] = (float)((double)x[(i << 1) + 1] / scale);
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	float f;
	for (size_t i = 0; i < n; i++)
	{
		if (jdsp->tmpBuffer[0][i] <= -1.0f)
			y[i << 1] = INT32_MIN;
		else if (jdsp->tmpBuffer[0][i] >= 1.0f)
			y[i << 1] = INT32_MAX;
		else
		{
			f = jdsp->tmpBuffer[0][i] * scale;
			y[i << 1] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		}
		if (jdsp->tmpBuffer[1][i] <= -1.0f)
			y[(i << 1) + 1] = INT32_MIN;
		else if (jdsp->tmpBuffer[1][i] >= 1.0f)
			y[(i << 1) + 1] = INT32_MAX;
		else
		{
			f = jdsp->tmpBuffer[1][i] * scale;
			y[(i << 1) + 1] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		}
	}
}
void pint8_24(JamesDSPLib *jdsp, int32_t *x1, int32_t *x2, int32_t *y1, int32_t *y2, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = (float)(1 << 23);
	float limpos = 0x7fffff / scale;
	float limneg = -0x800000 / scale;
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = (float)((double)x1[i] / scale);
		jdsp->tmpBuffer[1][i] = (float)((double)x2[i] / scale);
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	float f;
	for (size_t i = 0; i < n; i++)
	{
		f = jdsp->tmpBuffer[0][i] * scale;
		y1[i] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		f = jdsp->tmpBuffer[1][i] * scale;
		y2[i] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
	}
}
void pint8_24Multiplexed(JamesDSPLib *jdsp, int32_t *x, int32_t *y, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = (float)(1 << 23);
	float limpos = 0x7fffff / scale;
	float limneg = -0x800000 / scale;
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = (float)((double)x[i << 1] / scale);
		jdsp->tmpBuffer[1][i] = (float)((double)x[(i << 1) + 1] / scale);
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	float f;
	for (size_t i = 0; i < n; i++)
	{
		f = jdsp->tmpBuffer[0][i] * scale;
		y[i << 1] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
		f = jdsp->tmpBuffer[1][i] * scale;
		y[(i << 1) + 1] = (int32_t)(f > 0 ? f + 0.5f : f - 0.5f);
	}
}
void pintp24(JamesDSPLib *jdsp, uint8_t *x1, uint8_t *x2, uint8_t *y1, uint8_t *y2, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = 1.0f / (float)(1UL << 31);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = jdsp->i32_from_p24(x1 + i * 3) * scale;
		jdsp->tmpBuffer[1][i] = jdsp->i32_from_p24(x2 + i * 3) * scale;
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->p24_from_i32(clamp24_from_float(jdsp->tmpBuffer[0][i]), y1 + i * 3);
		jdsp->p24_from_i32(clamp24_from_float(jdsp->tmpBuffer[1][i]), y2 + i * 3);
	}
}
void pintp24Multiplexed(JamesDSPLib *jdsp, uint8_t *x, uint8_t *y, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	static const float scale = 1.0f / (float)(1UL << 31);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = jdsp->i32_from_p24(x + (i << 1) * 3) * scale;
		jdsp->tmpBuffer[1][i] = jdsp->i32_from_p24(x + ((i << 1) + 1) * 3) * scale;
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		jdsp->p24_from_i32(clamp24_from_float(jdsp->tmpBuffer[0][i]), y + (i << 1) * 3);
		jdsp->p24_from_i32(clamp24_from_float(jdsp->tmpBuffer[1][i]), y + ((i << 1) + 1) * 3);
	}
}
void pfloat32(JamesDSPLib *jdsp, float *x1, float *x2, float *y1, float *y2, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = x1[i];
		jdsp->tmpBuffer[1][i] = x2[i];
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		y1[i] = jdsp->tmpBuffer[0][i];
		y2[i] = jdsp->tmpBuffer[1][i];
	}
}
void pfloat32Multiplexed(JamesDSPLib *jdsp, float *x, float *y, size_t n)
{
	if (jdsp->blockSizeMax < n)
		JamesDSPReallocateBlock(jdsp, n);
	if (jdsp->blockSize != n)
	{
		jdsp->blockSize = n;
		JamesDSPRefreshConvolutions(jdsp, 1);
	}
	for (size_t i = 0; i < n; i++)
	{
		jdsp->tmpBuffer[0][i] = x[i << 1];
		jdsp->tmpBuffer[1][i] = x[(i << 1) + 1];
	}
	if (jdsp->enableASRC)
	{
		unsigned int curDecimatedLen = DoASRC_fwd(jdsp, n);
		jdsp->processInternal(jdsp, curDecimatedLen);
		DoASRC_bwd(jdsp, curDecimatedLen, n);
	}
	else
		jdsp->processInternal(jdsp, n);
	for (size_t i = 0; i < n; i++)
	{
		y[i << 1] = jdsp->tmpBuffer[0][i];
		y[(i << 1) + 1] = jdsp->tmpBuffer[1][i];
	}
}
extern void JamesDSPOfflineResampling(float const *in, float *out, size_t lenIn, size_t lenOut, int channels, double src_ratio);
// Binary blobs
extern const int hrtfLenPerChannel;
extern const int hrtfFs;
extern const int compressedLen_jdspImp;
extern const unsigned char jdspImp[7172];
extern const int compressedLen_CCConv;
extern const double ccconv1Gain;
extern const double ccconv2Gain;
extern const double ccconv3Gain;
extern const double ccconv4Gain;
extern const unsigned char CCConv[202119];
void JamesDSPRefreshBlob(JamesDSPLib *jdsp, double targetFs)
{
	const int channelsBlobsShort = 4;
	double ratio = targetFs / (double)hrtfFs;
	int outLen = (int)ceil(hrtfLenPerChannel * ratio);
	int i;
	if (jdsp->blobsCh1[0])
	{
		for (i = 0; i < 3; i++)
		{
			free(jdsp->blobsCh1[i]);
			free(jdsp->blobsCh2[i]);
			free(jdsp->blobsCh3[i]);
			free(jdsp->blobsCh4[i]);
		}
	}
	for (i = 0; i < 3; i++)
	{
		jdsp->blobsCh1[i] = (float *)malloc(outLen * sizeof(float));
		jdsp->blobsCh2[i] = (float *)malloc(outLen * sizeof(float));
		jdsp->blobsCh3[i] = (float *)malloc(outLen * sizeof(float));
		jdsp->blobsCh4[i] = (float *)malloc(outLen * sizeof(float));
	}
	jdsp->blobsResampledLen = outLen;
	float *tmpBuf = (float *)malloc(outLen * channelsBlobsShort * sizeof(float));
	memset(tmpBuf, 0, outLen * channelsBlobsShort * sizeof(float));

	drflac *pFlac = drflac_open_memory((void *)jdspImp, compressedLen_jdspImp, 0);
	size_t totalSmps = pFlac->totalPCMFrameCount * (size_t)pFlac->channels;
	float *pFrameImpulse = (float *)malloc(totalSmps * sizeof(float));
	size_t sampleCount = pFlac->totalPCMFrameCount;
	drflac_read_pcm_frames_f32(pFlac, totalSmps, pFrameImpulse);
	size_t copySize = (hrtfLenPerChannel << 2) * sizeof(float);
	float *CorredHRTF_Surround1 = (float *)malloc(copySize);
	memcpy(CorredHRTF_Surround1, pFrameImpulse, copySize);
	float *CorredHRTF_Surround2 = (float *)malloc(copySize);
	memcpy(CorredHRTF_Surround2, pFrameImpulse + (hrtfLenPerChannel << 2), copySize);
	float *CorredHRTFCrossfeed = (float *)malloc(copySize);
	memcpy(CorredHRTFCrossfeed, pFrameImpulse + ((hrtfLenPerChannel << 2) << 1), copySize);
	drflac_close(pFlac);
	free(pFrameImpulse);

	float *ptr1[4] = { jdsp->blobsCh1[0], jdsp->blobsCh2[0], jdsp->blobsCh3[0], jdsp->blobsCh4[0] };
	JamesDSPOfflineResampling(CorredHRTFCrossfeed, tmpBuf, hrtfLenPerChannel, outLen, channelsBlobsShort, ratio);
	free(CorredHRTFCrossfeed);
	channel_splitFloat(tmpBuf, outLen, ptr1, channelsBlobsShort);
	float *ptr2[4] = { jdsp->blobsCh1[1], jdsp->blobsCh2[1], jdsp->blobsCh3[1], jdsp->blobsCh4[1] };
	memset(tmpBuf, 0, outLen * channelsBlobsShort * sizeof(float));
	JamesDSPOfflineResampling(CorredHRTF_Surround1, tmpBuf, hrtfLenPerChannel, outLen, channelsBlobsShort, ratio);
	free(CorredHRTF_Surround1);
	channel_splitFloat(tmpBuf, outLen, ptr2, channelsBlobsShort);
	float *ptr3[4] = { jdsp->blobsCh1[2], jdsp->blobsCh2[2], jdsp->blobsCh3[2], jdsp->blobsCh4[2] };
	memset(tmpBuf, 0, outLen * channelsBlobsShort * sizeof(float));
	JamesDSPOfflineResampling(CorredHRTF_Surround2, tmpBuf, hrtfLenPerChannel, outLen, channelsBlobsShort, ratio);
	free(CorredHRTF_Surround2);
	channel_splitFloat(tmpBuf, outLen, ptr3, channelsBlobsShort);
	free(tmpBuf);

	pFlac = drflac_open_memory((void *)CCConv, compressedLen_CCConv, 0);
	ratio = targetFs / (double)(pFlac->sampleRate);
	totalSmps = pFlac->totalPCMFrameCount * (size_t)pFlac->channels;
	pFrameImpulse = (float *)malloc(totalSmps * sizeof(float));
	sampleCount = pFlac->totalPCMFrameCount;
	drflac_read_pcm_frames_f32(pFlac, totalSmps, pFrameImpulse);
	drflac_close(pFlac);
	outLen = (int)ceil(sampleCount * ratio);
	if (jdsp->hrtfblobsResampled[0])
		for (i = 0; i < 4; i++)
			free(jdsp->hrtfblobsResampled[i]);
	for (i = 0; i < 4; i++)
		jdsp->hrtfblobsResampled[i] = (float *)malloc(outLen * sizeof(float));
	jdsp->frameLenSVirResampled = outLen;
	tmpBuf = (float *)malloc(outLen * channelsBlobsShort * sizeof(float));
	memset(tmpBuf, 0, outLen * channelsBlobsShort * sizeof(float));
	JamesDSPOfflineResampling(pFrameImpulse, tmpBuf, sampleCount, outLen, channelsBlobsShort, ratio);
	free(pFrameImpulse);
	channel_splitFloat(tmpBuf, outLen, jdsp->hrtfblobsResampled, channelsBlobsShort);
	free(tmpBuf);
	for (i = 0; i < outLen; i++)
	{
		jdsp->hrtfblobsResampled[0][i] = (float)((double)jdsp->hrtfblobsResampled[0][i] * ccconv1Gain);
		jdsp->hrtfblobsResampled[1][i] = (float)((double)jdsp->hrtfblobsResampled[1][i] * ccconv2Gain);
		jdsp->hrtfblobsResampled[2][i] = (float)((double)jdsp->hrtfblobsResampled[2][i] * ccconv3Gain);
		jdsp->hrtfblobsResampled[3][i] = (float)((double)jdsp->hrtfblobsResampled[3][i] * ccconv4Gain);
	}
}
// Init JamesDSP
void JamesDSPInit(JamesDSPLib *jdsp, int n, float sample_rate)
{
	memset(jdsp, 0, sizeof(JamesDSPLib));
	// Endianness detection
	unsigned int x = 1;
	if ((((char *)&x)[0]) == 1)
	{
		jdsp->i32_from_p24 = i32_from_p24_little_endian;
		jdsp->p24_from_i32 = p24_from_i32_little_endian;
	}
	else
	{
		jdsp->i32_from_p24 = i32_from_p24_big_endian;
		jdsp->p24_from_i32 = p24_from_i32_big_endian;
	}
	//
	if (pthread_mutex_init(&jdsp->m_in_processing, NULL) != 0)
		jdsp->isMutexSuccess = 0;
	else
		jdsp->isMutexSuccess = 1;
	// Init buffer
	jdsp->blockSize = n;
	jdsp->blockSizeMax = n;
	// Function pointer
	jdsp->processInt16Deinterleaved = pint16;
	jdsp->processInt32Deinterleaved = pint32;
	jdsp->processFloatDeinterleaved = pfloat32;
	jdsp->processInt16Multiplexd = pint16Multiplexed;
	jdsp->processInt32Multiplexd = pint32Multiplexed;
	jdsp->processFloatMultiplexd = pfloat32Multiplexed;
	jdsp->processInt8_24Multiplexd = pint8_24Multiplexed;
	jdsp->processInt24PackedMultiplexd = pintp24Multiplexed;
	jdsp->processInt8_24Deinterleaved = pint8_24;
	jdsp->processInt24PackedDeinterleaved = pintp24;
	jdsp->processInternal = JamesDSPProcessCheckBenchmarkReady;
	//
	const unsigned int asrc_taps = 64;
	char isminphase = 1;
	if (sample_rate < 44100.0f || sample_rate > 48000.0f)
	{
		jdsp->enableASRC = 1;
		int roundedRate = (int)(sample_rate);
		jdsp->trueSampleRate = sample_rate;
		if (((roundedRate % 48000 == 0) || (48000 % roundedRate == 0)) && roundedRate != 48000)
			jdsp->fs = 48000;
		else if (((roundedRate % 44100 == 0) || (44100 % roundedRate == 0)) && roundedRate != 44100)
			jdsp->fs = 44100;
		else
			jdsp->fs = 48000;
		InitIntegerASRCHandler(&jdsp->asrc[0], (unsigned long long)jdsp->fs, (unsigned long long)jdsp->trueSampleRate, asrc_taps, isminphase, 0, 0);
		InitIntegerASRCHandler(&jdsp->asrc[1], (unsigned long long)jdsp->fs, (unsigned long long)jdsp->trueSampleRate, asrc_taps, isminphase, &jdsp->asrc[0].polyphaseDecimator, &jdsp->asrc[0].polyphaseInterpolator);
		double ratio = (double)jdsp->fs / (double)jdsp->trueSampleRate;
		unsigned int maxDecimatedLength = (unsigned int)ceil(n * ratio);
		unsigned int maxInterpolatedLength = (unsigned int)ceil(maxDecimatedLength / ratio);
		jdsp->pw2BlockMemSize = next_pow_2(maxInterpolatedLength);
		size_t ctMemBlk = jdsp->blockSizeMax * 2 + maxInterpolatedLength * 2 + jdsp->pw2BlockMemSize * 2;
		jdsp->tmpBuffer[0] = (float *)malloc(ctMemBlk * sizeof(float));
		jdsp->tmpBuffer[1] = jdsp->tmpBuffer[0] + jdsp->blockSizeMax;
		jdsp->tmpBuffer[2] = jdsp->tmpBuffer[1] + jdsp->blockSizeMax;
		jdsp->tmpBuffer[3] = jdsp->tmpBuffer[2] + maxInterpolatedLength;
		jdsp->tmpBuffer[4] = jdsp->tmpBuffer[3] + maxInterpolatedLength;
		jdsp->tmpBuffer[5] = jdsp->tmpBuffer[4] + jdsp->pw2BlockMemSize;
	}
	else
	{
		size_t ctMemBlk = jdsp->blockSizeMax * 2;
		jdsp->tmpBuffer[0] = (float *)malloc(ctMemBlk * sizeof(float));
		jdsp->tmpBuffer[1] = jdsp->tmpBuffer[0] + jdsp->blockSizeMax;
		jdsp->trueSampleRate = sample_rate;
		jdsp->fs = sample_rate;
		jdsp->enableASRC = 0;
	}
	// Init IO control
	JLimiterInit(jdsp);
	JLimiterSetCoefficients(jdsp, -(double)(FLT_EPSILON * 10.0f), 100.0);
	jdsp->postGain = 1.0f;
	// Init effect
	LiveProgConstructor(jdsp);
	CompressorConstructor(jdsp);
	CompressorDisable(jdsp);
	BassBoostConstructor(jdsp);
	BassBoostDisable(jdsp);
	ReverbDisable(jdsp);
	StereoEnhancementConstructor(jdsp);
	StereoEnhancementDisable(jdsp);
	VacuumTubeDisable(jdsp);
	LiveProgDisable(jdsp);
	DDCConstructor(jdsp);
	DDCDisable(jdsp);
	CrossfeedConstructor(jdsp);
	CrossfeedDisable(jdsp);
	Convolver1DConstructor(jdsp);
	ArbitraryResponseEqualizerConstructor(jdsp);
	ArbitraryResponseEqualizerDisable(jdsp);
	MultimodalEqualizerConstructor(jdsp);
	MultimodalEqualizerDisable(jdsp);
	// Init binary blobs, random number generator
	jdsp->rndstate[0] = time(0);
	for (int i = 0; i < 3; i++)
	{
		jdsp->blobsCh1[i] = 0;
		jdsp->blobsCh2[i] = 0;
		jdsp->blobsCh3[i] = 0;
		jdsp->blobsCh4[i] = 0;
		for (int j = 0; j < ((n > 1) ? (n / (i + 1)) : (128)); j++)
			randXorshift(jdsp->rndstate);
	}
	JamesDSPRefreshBlob(jdsp, (double)jdsp->fs);
	jdsp->rndstate[1] = (uint64_t)(randXorshift(jdsp->rndstate) * 2.0);
#ifdef DEBUG
	__android_log_print(ANDROID_LOG_INFO, TAG, "Printing benchmark data start");
	for (int s = 0; s < MAX_BENCHMARK; s++)
		__android_log_print(ANDROID_LOG_INFO, TAG, "%1.7lf,%1.7lf", convbench_c0[s], convbench_c1[s]);
	__android_log_print(ANDROID_LOG_INFO, TAG, "Printing benchmark data end");
#endif
}
void JamesDSPSetPostGain(JamesDSPLib *jdsp, double pGaindB)
{
	if (pGaindB < -15.0f)
		pGaindB = -15.0f;
	if (pGaindB > 15.0f)
		pGaindB = 15.0f;
	jdsp->postGain = db2magf(pGaindB);
}
int JamesDSPGetMutexStatus(JamesDSPLib *jdsp)
{
	return jdsp->isMutexSuccess;
}
void JamesDSPSetSampleRate(JamesDSPLib *jdsp, float new_sample_rate, int forceRefresh)
{
	if (jdsp->trueSampleRate == new_sample_rate)
		return;
	jdsp->trueSampleRate = new_sample_rate;
	jdsp_lock(jdsp);
	if (jdsp->enableASRC)
	{
		FreeIntegerASRCHandler(&jdsp->asrc[0]);
		FreeIntegerASRCHandler(&jdsp->asrc[1]);
	}
	const unsigned int asrc_taps = 32;
	char isminphase = 1;
	if (new_sample_rate < 44100.0f || new_sample_rate > 48000.0f)
	{
		jdsp->enableASRC = 1;
		int roundedRate = (int)(new_sample_rate);
		if (((roundedRate % 48000 == 0) || (48000 % roundedRate == 0)) && roundedRate != 48000)
			jdsp->fs = 48000;
		else if (((roundedRate % 44100 == 0) || (44100 % roundedRate == 0)) && roundedRate != 44100)
			jdsp->fs = 44100;
		else
			jdsp->fs = 48000;
		InitIntegerASRCHandler(&jdsp->asrc[0], (unsigned long long)jdsp->fs, (unsigned long long)jdsp->trueSampleRate, asrc_taps, isminphase, 0, 0);
		InitIntegerASRCHandler(&jdsp->asrc[1], (unsigned long long)jdsp->fs, (unsigned long long)jdsp->trueSampleRate, asrc_taps, isminphase, &jdsp->asrc[0].polyphaseDecimator, &jdsp->asrc[0].polyphaseInterpolator);
		double ratio = (double)jdsp->fs / (double)jdsp->trueSampleRate;
		unsigned int maxDecimatedLength = (unsigned int)ceil(jdsp->blockSizeMax * ratio);
		unsigned int maxInterpolatedLength = (unsigned int)ceil(maxDecimatedLength / ratio);
		jdsp->pw2BlockMemSize = next_pow_2(maxInterpolatedLength);
		if (jdsp->tmpBuffer[0])
			free(jdsp->tmpBuffer[0]);
		size_t ctMemBlk = jdsp->blockSizeMax * 2 + maxInterpolatedLength * 2 + jdsp->pw2BlockMemSize * 2;
		jdsp->tmpBuffer[0] = (float *)malloc(ctMemBlk * sizeof(float));
		jdsp->tmpBuffer[1] = jdsp->tmpBuffer[0] + jdsp->blockSizeMax;
		jdsp->tmpBuffer[2] = jdsp->tmpBuffer[1] + jdsp->blockSizeMax;
		jdsp->tmpBuffer[3] = jdsp->tmpBuffer[2] + maxInterpolatedLength;
		jdsp->tmpBuffer[4] = jdsp->tmpBuffer[3] + maxInterpolatedLength;
		jdsp->tmpBuffer[5] = jdsp->tmpBuffer[4] + jdsp->pw2BlockMemSize;
	}
	else
	{
		jdsp->enableASRC = 0;
		jdsp->fs = jdsp->trueSampleRate;
	}
	JamesDSPRefreshBlob(jdsp, jdsp->fs);
	if (forceRefresh)
	{
		BassBoostSetParam(jdsp, jdsp->dbb.maxGain);
		jdsp->ddcForceRefresh = 1;
		DDCEnable(jdsp, jdsp->ddcEnabled);
		jdsp->crossfeedForceRefresh = 1;
		CrossfeedEnable(jdsp, jdsp->crossfeedEnabled);
		jdsp->arbMagForceRefresh = 1;
		ArbitraryResponseEqualizerEnable(jdsp, jdsp->arbitraryMagEnabled);
		jdsp->equalizerForceRefresh = 1;
		MultimodalEqualizerEnable(jdsp, jdsp->equalizerEnabled);
		jdsp->compForceRefresh = 1;
		CompressorEnable(jdsp, jdsp->compEnabled);
		StereoEnhancementRefresh(jdsp);
	}
	jdsp_unlock(jdsp);
}
void JamesDSPFree(JamesDSPLib *jdsp)
{
	jdsp_lock(jdsp);
	StereoEnhancementDestructor(jdsp);
	CompressorDestructor(jdsp);
	LiveProgDestructor(jdsp);
	DDCDestructor(jdsp);
	CrossfeedDestructor(jdsp);
	Convolver1DDestructor(jdsp);
	ArbitraryResponseEqualizerDestructor(jdsp);
	MultimodalEqualizerDestructor(jdsp);
	if (jdsp->tmpBuffer[0])
		free(jdsp->tmpBuffer[0]);
	int i;
	if (jdsp->blobsCh1[0])
	{
		for (i = 0; i < 3; i++)
		{
			free(jdsp->blobsCh1[i]);
			free(jdsp->blobsCh2[i]);
			free(jdsp->blobsCh3[i]);
			free(jdsp->blobsCh4[i]);
		}
	}
	if (jdsp->hrtfblobsResampled[0])
	{
		for (i = 0; i < 4; i++)
			free(jdsp->hrtfblobsResampled[i]);
	}
	if (jdsp->impulseResponseStorage.impulseResponse)
		free(jdsp->impulseResponseStorage.impulseResponse);
	if (jdsp->isMutexSuccess)
		pthread_mutex_destroy(&jdsp->m_in_processing);
	if (jdsp->enableASRC)
	{
		FreeIntegerASRCHandler(&jdsp->asrc[0]);
		FreeIntegerASRCHandler(&jdsp->asrc[1]);
	}
	jdsp_unlock(jdsp);
}
