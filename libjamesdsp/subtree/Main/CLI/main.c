#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../libjamesdsp/jni/jamesdsp/jdsp/jdsp_header.h"
#include "../libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/dr_wav.h"
#include <tchar.h>
char *openTextFile(char *fileSelectorPath)
{
	char *buffer = 0;
	long length;
	FILE *textFile = fopen(fileSelectorPath, "rb");
	if (textFile)
	{
		fseek(textFile, 0, SEEK_END);
		length = ftell(textFile);
		fseek(textFile, 0, SEEK_SET);
		buffer = (char *)malloc(length + 1);
		if (buffer)
			fread(buffer, 1, length, textFile);
		fclose(textFile);
		buffer[length] = '\0';
	}
	return buffer;
}
extern void JamesDSPOfflineResampling(float const *in, float *out, size_t lenIn, size_t lenOut, int channels, double src_ratio);
float *FileSelecterPopupWavFileWin32(char *fileSelectorPath, double targetFs, unsigned int *channels, size_t *totalPCMFrameCount)
{
	unsigned int fs;
	float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(fileSelectorPath, channels, &fs, totalPCMFrameCount, 0);
	if (pSampleData == NULL)
	{
		printf("Error opening and reading WAV file");
		return 0;
	}
	// Sanity check
	if (*channels < 1)
	{
		printf("Invalid audio channels count");
		free(pSampleData);
		return 0;
	}
	if ((*totalPCMFrameCount <= 0) || (*totalPCMFrameCount <= 0))
	{
		printf("Invalid audio sample rate / frame count");
		free(pSampleData);
		return 0;
	}
	double ratio = targetFs / (double)fs;
	int compressedLen = (int)ceil(*totalPCMFrameCount * ratio);
	float *tmpBuf = (float *)malloc(compressedLen * *channels * sizeof(float));
	memset(tmpBuf, 0, compressedLen * *channels * sizeof(float));
	JamesDSPOfflineResampling(pSampleData, tmpBuf, *totalPCMFrameCount, compressedLen, *channels, ratio);
	*totalPCMFrameCount = compressedLen;
	free(pSampleData);
	return tmpBuf;
}
char *basename(char const *path)
{
#ifdef _MSC_VER
	char *s = strrchr(path, '\\');
#else
	char *s = strrchr(path, '/');
#endif
	if (!s)
		return _strdup(path);
	else
		return _strdup(s + 1);
}
// Thread safety test
typedef struct
{
	JamesDSPLib *dspPtr;
	int paramInt;
	float paramFloat;
} threadTest;
/* this function is run by the second thread */
float *loadWaveFile(double targetFs, unsigned int *channels, size_t *totalPCMFrameCount)
{
	unsigned int fs;
	float *pSampleData = drwav_open_file_and_read_pcm_frames_f32("EqResponse.wav", channels, &fs, totalPCMFrameCount, 0);
	if (pSampleData == NULL)
	{
		printf("Error opening and reading WAV file");
		return 0;
	}
	// Sanity check
	if (*channels < 1)
	{
		printf("Invalid audio channels count");
		free(pSampleData);
		return 0;
	}
	if ((*totalPCMFrameCount <= 0) || (*totalPCMFrameCount <= 0))
	{
		printf("Invalid audio sample rate / frame count");
		free(pSampleData);
		return 0;
	}
	double ratio = targetFs / (double)fs;
	int compressedLen = (int)ceil(*totalPCMFrameCount * ratio);
	float *tmpBuf = (float *)malloc(compressedLen * *channels * sizeof(float));
	memset(tmpBuf, 0, compressedLen * *channels * sizeof(float));
	JamesDSPOfflineResampling(pSampleData, tmpBuf, *totalPCMFrameCount, compressedLen, *channels, ratio);
	*totalPCMFrameCount = compressedLen;
	free(pSampleData);
	return tmpBuf;
}
void *inc_x(void *x_void_ptr)
{
	threadTest *x_ptr = (threadTest *)x_void_ptr;
	JamesDSPLib *jdsp = x_ptr->dspPtr;
	//	jdsp->crossfeedForceRefresh = 1;
	//	CrossfeedChangeMode(jdsp, 3);
	int impulseChannels;
	size_t impFrameCount;
	float *impulseResponse = loadWaveFile(jdsp->fs, &impulseChannels, &impFrameCount);
	Convolver1DLoadImpulseResponse(jdsp, impulseResponse, impulseChannels, impFrameCount, 1);
	free(impulseResponse);
	Convolver1DEnable(jdsp);
	pthread_exit(NULL);
	return NULL;
}
double freqTh[NUMPTS] = { 25.0, 40.0, 63.0, 100.0, 160.0, 250.0, 400.0, 630.0, 1000.0, 1600.0, 2500.0, 4000.0, 6300.0, 10000.0, 16000.0 };
double gainsTh[NUMPTS] = { 5.0, -1.0, -4, -1, 2.1, 0.0, 0.0, 0.7, -10.7, 0.0, 0.0, 0.0, 0.0, 0.8, 8.0 };
int idxN, readCountN;
/*void *inc_eq(void *x_void_ptr)
{
	threadTest *x_ptr = (threadTest *)x_void_ptr;
	JamesDSPLib *jdsp = x_ptr->dspPtr;
	int ii = 0;
	while (1)
	{
		if (idxN <= ii)
			continue;
		ii = idxN;
		if (idxN > 0 && idxN < 500)
		{
			gainsTh[5] += 0.48;
			if (gainsTh[5] > 64)
				gainsTh[5] = 64;
			MultimodalEqualizerAxisInterpolation(jdsp, 0, 5, freqTh, gainsTh);
		}
		else if (idxN >= 500)
		{
			gainsTh[5] -= 0.4;
			if (gainsTh[5] < -64)
				gainsTh[5] = -64;
			gainsTh[10] -= 0.1;
			if (gainsTh[10] < -64)
				gainsTh[10] = -64;
			MultimodalEqualizerAxisInterpolation(jdsp, 0, 5, freqTh, gainsTh);
		}
		if (idxN >= readCountN - 1)
			break;
	}
	pthread_exit(NULL);
	return NULL;
}*/
#define FILENAME "mus.wav"
static inline float float_from_i32(int32_t ival)
{
	static const float scale = 1. / (float)(1UL << 31);
	return ival * scale;
}
int32_t clamp24_from_float(float f);
int main()
{
	char oldPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, oldPath);
	char *text = 0;
	int i;
	unsigned int channels;
	unsigned int sampleRate;
	drwav_uint64 totalPCMFrameCount;
	float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(FILENAME, &channels, &sampleRate, &totalPCMFrameCount, 0);
	if (pSampleData == NULL)
	{
		printf("Error opening and reading WAV file");
		return -1;
	}
	// Sanity check
	if (channels < 1)
	{
		printf("Invalid audio channels count");
		free(pSampleData);
		return -1;
	}
	if ((totalPCMFrameCount <= 0) || (totalPCMFrameCount <= 0))
	{
		printf("Invalid audio sample rate / frame count");
		free(pSampleData);
		return -1;
	}
	char *filename = basename(FILENAME);
	int frameCountBest = 1024;
	printf("[Status] Precomputing...\n");
	JamesDSPGlobalMemoryAllocation();
	JamesDSPLib *jdsp = (JamesDSPLib *)malloc(sizeof(JamesDSPLib));
	memset(jdsp, 0, sizeof(JamesDSPLib));
	JamesDSPInit(jdsp, 77, sampleRate);
	JLimiterSetCoefficients(jdsp, 0.0, 100);
	// Parameters
	// DRC
	double freqDRC[NUMPTS_DRS] = { 95.0, 200.0, 400.0, 800.0, 1600.0, 3400.0, 7500.0 };
	double gainsDRC[NUMPTS_DRS] = { 1.0, 0.9, 0.2, 0.0, 0.0, 0.8, 1.0 };
	for (i = 0; i < NUMPTS_DRS; i++)
		gainsDRC[i] = -1.0;
	CompressorSetParam(jdsp, 0.8, 4, 1, 1);
	CompressorSetGain(jdsp, freqDRC, gainsDRC, 1);
	CompressorEnable(jdsp, 1);
	// Bass boost
	//BassBoostSetParam(jdsp, 10.0f);
	//BassBoostEnable(jdsp);
	// Reverb
	//Reverb_SetParam(jdsp, 4);
	//ReverbEnable(jdsp);
	// Stereo enhancement
	//StereoEnhancementSetParam(jdsp, 0.9f);
	//StereoEnhancementEnable(jdsp);
	// Vacuum tube
	//VacuumTubeSetGain(jdsp, 8.0);
	//VacuumTubeEnable(jdsp);
	// Live programmable DSP
	/*text = openTextFile("hadamVerb.eel");
	if (text)
	{
		LiveProgStringParser(jdsp, text);
		free(text);
	}
	LiveProgEnable(jdsp);*/
	// DDC
	/*text = openTextFile("Beyerdynamic DT770-80-4.vdc");
	if (text)
	{
		DDCStringParser(jdsp, text);
		free(text);
	}
	text = openTextFile("Butterworth.vdc");
	if (text)
	{
		DDCStringParser(jdsp, text);
		free(text);
	}
	text = openTextFile("FrontRearContrast.vdc");
	if (text)
	{
		DDCStringParser(jdsp, text);
		free(text);
	}
	DDCEnable(jdsp, 1);*/
	// Crossfeed
	/*CrossfeedChangeMode(jdsp, 5);
	CrossfeedChangeMode(jdsp, 3);
	CrossfeedEnable(jdsp, 1);*/
	// Convolver
	int impulseChannels;
	size_t impFrameCount;
	//
	/*float *impulseResponse = FileSelecterPopupWavFileWin32("longReverb.wav", jdsp->fs, &impulseChannels, &impFrameCount);
	Convolver1DLoadImpulseResponse(jdsp, impulseResponse, impulseChannels, impFrameCount, 1);
	free(impulseResponse);*/
	//
	/*impulseResponse = FileSelecterPopupWavFileWin32("EqResponse.wav", jdsp->fs, &impulseChannels, &impFrameCount);
	Convolver1DLoadImpulseResponse(jdsp, impulseResponse, impulseChannels, impFrameCount, 1);
	free(impulseResponse);
	//
	impulseResponse = FileSelecterPopupWavFileWin32("408.wav", jdsp->fs, &impulseChannels, &impFrameCount);
	Convolver1DLoadImpulseResponse(jdsp, impulseResponse, impulseChannels, impFrameCount, 1);
	free(impulseResponse);*/
	//Convolver1DEnable(jdsp);
	// Arbitrary magnitude response
	/*ArbitraryResponseEqualizerStringParser(jdsp, "IE80Cal: 10 -6.1681; 10.3 -6.31; 10.6247 -6.339; 10.7171 -6.2821; 10.8103 -6.396; 10.9043 -6.396; 10.9991 -6.51; 11.0947 -6.51; 11.1912 -6.51; 11.2885 -6.51; 11.3866 -6.6239; 11.4856 -6.6239; 11.5855 -6.6239; 11.6862 -6.7379; 11.7878 -6.7379; 11.8903 -6.7379; 11.9937 -6.7379; 12.098 -6.8519; 12.2032 -6.8519; 12.3093 -6.8519; 12.4163 -6.9658; 12.5243 -6.9658; 12.6332 -6.9658; 12.743 -6.9658; 12.8538 -7.0798; 12.9656 -7.0798; 13.0783 -7.0798; 13.192 -7.1937; 13.3067 -7.1937; 13.4224 -7.1937; 13.5391 -7.1937; 13.6568 -7.3077; 13.7756 -7.3077; 13.8953 -7.3077; 14.0162 -7.4217; 14.138 -7.4217; 14.2609 -7.4217; 14.3849 -7.4217; 14.51 -7.5356; 14.6362 -7.5356; 14.7634 -7.5356; 14.8918 -7.6496; 15.0213 -7.6496; 15.1519 -7.6496; 15.2836 -7.6496; 15.4165 -7.7635; 15.5505 -7.7635; 15.6857 -7.7635; 15.8221 -7.8775; 15.9597 -7.8775; 16.0985 -7.8775; 16.2384 -7.8775; 16.3796 -7.9915; 16.522 -7.9915; 16.6657 -7.9915; 16.8106 -8.1054; 16.9568 -8.1054; 17.1042 -8.1054; 17.2529 -8.1054; 17.4029 -8.2194; 17.5542 -8.2194; 17.7068 -8.2194; 17.8608 -8.2194; 18.0161 -8.3333; 18.1727 -8.4473; 18.3307 -8.4473; 18.4901 -8.4473; 18.6509 -8.4473; 18.8131 -8.4473; 18.9766 -8.5613; 19.1416 -8.5613; 19.308 -8.5613; 19.4759 -8.5613; 19.6453 -8.5613; 19.8161 -8.5613; 19.9884 -8.6752; 20.1622 -8.6752; 20.3375 -8.6752; 20.5143 -8.7892; 20.6926 -8.7892; 20.8726 -8.7892; 21.054 -8.7892; 21.2371 -8.9031; 21.4217 -8.9031; 21.608 -8.9031; 21.7959 -8.9031; 21.9854 -8.9031; 22.1765 -9.0171; 22.3694 -9.0171; 22.5639 -9.0171; 22.76 -9.0171; 22.9579 -9.1311; 23.1575 -9.1311; 23.3589 -9.1311; 23.562 -9.1311; 23.7668 -9.1311; 23.9735 -9.245; 24.1819 -9.245; 24.3922 -9.245; 24.6043 -9.359; 24.8182 -9.359; 25.034 -9.359; 25.2516 -9.359; 25.4712 -9.4729; 25.6927 -9.4729; 25.916 -9.4729; 26.1414 -9.4729; 26.3687 -9.4729; 26.5979 -9.5869; 26.8292 -9.5869; 27.0625 -9.5869; 27.2978 -9.5869; 27.5351 -9.7009; 27.7745 -9.7009; 28.016 -9.7009; 28.2596 -9.7009; 28.5053 -9.7009; 28.7531 -9.7009; 29.0031 -9.7009; 29.2553 -9.8148; 29.5097 -9.8148; 29.7662 -9.9288; 30.0251 -9.9288; 30.2861 -9.9288; 30.5494 -9.9288; 30.8151 -9.9288; 31.083 -9.9288; 31.3532 -9.9288; 31.6258 -10.0427; 31.9008 -10.0427; 32.1782 -10.0427; 32.458 -10.0427; 32.7402 -10.0427; 33.0248 -10.1567; 33.312 -10.1567; 33.6016 -10.1567; 33.8938 -10.1567; 34.1885 -10.1567; 34.4857 -10.2707; 34.7856 -10.2707; 35.088 -10.2707; 35.3931 -10.2707; 35.7008 -10.3846; 36.0112 -10.3846; 36.3243 -10.3846; 36.6402 -10.3846; 36.9587 -10.3846; 37.2801 -10.3846; 37.6042 -10.3846; 37.9312 -10.4986; 38.261 -10.4986; 38.5936 -10.4986; 38.9292 -10.4986; 39.2677 -10.4986; 39.6091 -10.4986; 39.9535 -10.4986; 40.3008 -10.6125; 40.6512 -10.6125; 41.0047 -10.6125; 41.3612 -10.6125; 41.7208 -10.6125; 42.0836 -10.7265; 42.4495 -10.7265; 42.8186 -10.7265; 43.1909 -10.7265; 43.5664 -10.7265; 43.9452 -10.7265; 44.3273 -10.7265; 44.7127 -10.8405; 45.1014 -10.8405; 45.4936 -10.8405; 45.8891 -10.8405; 46.2881 -10.8405; 46.6906 -10.8405; 47.0965 -10.8405; 47.506 -10.9544; 47.9191 -10.9544; 48.3357 -10.9544; 48.756 -10.9544; 49.1799 -10.9544; 49.6075 -10.9544; 50.0388 -10.9544; 50.4739 -10.9544; 50.9127 -10.9544; 51.3554 -11.0684; 51.8019 -11.0684; 52.2523 -11.0684; 52.7066 -11.0684; 53.1649 -11.0684; 53.6272 -11.0684; 54.0934 -11.0684; 54.5637 -11.0684; 55.0382 -11.0684; 55.5167 -11.1823; 55.9994 -11.1823; 56.4863 -11.1823; 56.9774 -11.1823; 57.4728 -11.1823; 57.9725 -11.1823; 58.4766 -11.1823; 58.985 -11.1823; 59.4979 -11.1823; 60.0152 -11.1823; 60.537 -11.1823; 61.0633 -11.1823; 61.5943 -11.2963; 62.1298 -11.2963; 62.67 -11.2963; 63.2149 -11.2963; 63.7645 -11.2963; 64.3189 -11.2963; 64.8782 -11.2963; 65.4422 -11.2963; 66.0112 -11.2963; 66.5852 -11.2963; 67.1641 -11.2963; 67.7481 -11.2963; 68.3371 -11.2963; 68.9313 -11.2963; 69.5306 -11.4103; 70.1352 -11.4103; 70.745 -11.4103; 71.3601 -11.4103; 71.9805 -11.4103; 72.6064 -11.4103; 73.2377 -11.4103; 73.8744 -11.4103; 74.5168 -11.4103; 75.1647 -11.4103; 75.8182 -11.4103; 76.4774 -11.4103; 77.1423 -11.4103; 77.8131 -11.4103; 78.4896 -11.4103; 79.1721 -11.4103; 79.8604 -11.4103; 80.5548 -11.4103; 81.2552 -11.4103; 81.9617 -11.4103; 82.6743 -11.4103; 83.3931 -11.4103; 84.1182 -11.4103; 84.8496 -11.4103; 85.5873 -11.4103; 86.3315 -11.4103; 87.0821 -11.4103; 87.8392 -11.4103; 88.603 -11.5242; 89.3733 -11.5242; 90.1504 -11.5242; 90.9342 -11.5242; 91.7249 -11.5242; 92.5224 -11.5242; 93.3268 -11.5242; 94.1383 -11.5242; 94.9568 -11.5242; 95.7824 -11.5242; 96.6152 -11.5242; 97.4552 -11.5242; 98.3026 -11.5242; 99.1573 -11.5242; 100.019 -11.4103; 100.889 -11.4103; 101.766 -11.4103; 102.651 -11.4103; 103.544 -11.4103; 104.444 -11.4103; 105.352 -11.4103; 106.268 -11.4103; 107.192 -11.4103; 108.124 -11.4103; 109.064 -11.4103; 110.012 -11.4103; 110.969 -11.4103; 111.934 -11.4103; 112.907 -11.4103; 113.889 -11.4103; 114.879 -11.4103; 115.878 -11.4103; 116.885 -11.4103; 117.901 -11.4103; 118.926 -11.4103; 119.961 -11.4103; 121.004 -11.4103; 122.056 -11.4103; 123.117 -11.4103; 124.187 -11.4103; 125.267 -11.4103; 126.356 -11.4103; 127.455 -11.4103; 128.563 -11.2963; 129.681 -11.2963; 130.808 -11.2963; 131.946 -11.2963; 133.093 -11.2963; 134.25 -11.2963; 135.417 -11.2963; 136.595 -11.2963; 137.782 -11.2963; 138.98 -11.2963; 140.189 -11.2963; 141.408 -11.2963; 142.637 -11.2963; 143.877 -11.2963; 145.128 -11.1823; 146.39 -11.1823; 147.663 -11.1823; 148.947 -11.1823; 150.242 -11.1823; 151.548 -11.1823; 152.866 -11.1823; 154.195 -11.1823; 155.536 -11.1823; 156.888 -11.1823; 158.252 -11.0684; 159.628 -11.0684; 161.016 -11.0684; 162.416 -11.0684; 163.828 -11.0684; 165.252 -11.0684; 166.689 -11.0684; 168.139 -11.0684; 169.6 -11.0684; 171.075 -10.9544; 172.562 -10.9544; 174.063 -10.9544; 175.576 -10.9544; 177.103 -10.9544; 178.643 -10.9544; 180.196 -10.9544; 181.763 -10.8405; 183.343 -10.8405; 184.937 -10.8405; 186.545 -10.8405; 188.167 -10.8405; 189.803 -10.8405; 191.453 -10.8405; 193.118 -10.7265; 194.797 -10.7265; 196.491 -10.7265; 198.199 -10.7265; 199.922 -10.7265; 201.661 -10.7265; 203.414 -10.7265; 205.183 -10.6125; 206.967 -10.6125; 208.766 -10.6125; 210.581 -10.6125; 212.412 -10.6125; 214.259 -10.4986; 216.122 -10.4986; 218.001 -10.4986; 219.897 -10.4986; 221.808 -10.4986; 223.737 -10.4986; 225.682 -10.4986; 227.645 -10.4986; 229.624 -10.3846; 231.62 -10.3846; 233.634 -10.3846; 235.666 -10.3846; 237.715 -10.3846; 239.781 -10.2707; 241.866 -10.2707; 243.969 -10.2707; 246.09 -10.2707; 248.23 -10.2707; 250.388 -10.1567; 252.565 -10.1567; 254.761 -10.1567; 256.976 -10.1567; 259.211 -10.0427; 261.464 -10.0427; 263.738 -10.0427; 266.031 -10.0427; 268.344 -10.0427; 270.677 -9.9288; 273.031 -9.9288; 275.404 -9.9288; 277.799 -9.9288; 280.214 -9.9288; 282.651 -9.8148; 285.108 -9.8148; 287.587 -9.8148; 290.088 -9.8148; 292.61 -9.7009; 295.154 -9.7009; 297.72 -9.7009; 300.309 -9.5869; 302.92 -9.5869; 305.554 -9.5869; 308.21 -9.5869; 310.89 -9.5869; 313.593 -9.5869; 316.32 -9.4729; 319.07 -9.4729; 321.844 -9.4729; 324.643 -9.359; 327.465 -9.359; 330.312 -9.359; 333.184 -9.359; 336.081 -9.245; 339.003 -9.245; 341.951 -9.245; 344.924 -9.1311; 347.923 -9.1311; 350.948 -9.1311; 354 -9.1311; 357.077 -9.0171; 360.182 -9.0171; 363.314 -9.0171; 366.473 -8.9031; 369.659 -8.9031; 372.873 -8.9031; 376.115 -8.9031; 379.385 -8.7892; 382.684 -8.7892; 386.011 -8.7892; 389.367 -8.7892; 392.753 -8.7892; 396.168 -8.6752; 399.612 -8.6752; 403.087 -8.6752; 406.591 -8.5613; 410.127 -8.5613; 413.692 -8.5613; 417.289 -8.5613; 420.918 -8.5613; 424.577 -8.5613; 428.269 -8.4473; 431.992 -8.4473; 435.748 -8.4473; 439.537 -8.4473; 443.359 -8.4473; 447.214 -8.3333; 451.102 -8.3333; 455.024 -8.3333; 458.98 -8.3333; 462.971 -8.2194; 466.996 -8.2194; 471.057 -8.2194; 475.152 -8.2194; 479.284 -8.2194; 483.451 -8.1054; 487.654 -8.1054; 491.894 -8.1054; 496.171 -7.9915; 500.485 -7.9915; 504.837 -7.9915; 509.226 -7.9915; 513.654 -7.8775; 518.12 -7.8775; 522.625 -7.8775; 527.169 -7.7635; 531.752 -7.7635; 536.376 -7.7635; 541.039 -7.7635; 545.743 -7.6496; 550.488 -7.6496; 555.275 -7.6496; 560.103 -7.5356; 564.973 -7.5356; 569.885 -7.5356; 574.84 -7.5356; 579.838 -7.4217; 584.879 -7.4217; 589.965 -7.4217; 595.094 -7.4217; 600.268 -7.3077; 605.487 -7.3077; 610.752 -7.3077; 616.062 -7.1937; 621.418 -7.1937; 626.822 -7.1937; 632.272 -7.1937; 637.769 -7.1937; 643.314 -7.0798; 648.907 -7.0798; 654.549 -7.0798; 660.241 -7.0798; 665.981 -6.9658; 671.772 -6.9658; 677.612 -6.9658; 683.504 -6.9658; 689.447 -6.9658; 695.441 -6.8519; 701.488 -6.8519; 707.587 -6.8519; 713.739 -6.8519; 719.945 -6.8519; 726.205 -6.7379; 732.519 -6.7379; 738.888 -6.7379; 745.312 -6.7379; 751.792 -6.7379; 758.329 -6.7379; 764.922 -6.7379; 771.573 -6.6239; 778.282 -6.6239; 785.049 -6.6239; 791.874 -6.6239; 798.759 -6.6239; 805.704 -6.6239; 812.71 -6.6239; 819.776 -6.6239; 826.903 -6.6239; 834.093 -6.51; 841.345 -6.51; 848.66 -6.51; 856.039 -6.51; 863.482 -6.51; 870.99 -6.51; 878.563 -6.51; 886.202 -6.51; 893.907 -6.51; 901.679 -6.51; 909.519 -6.51; 917.427 -6.51; 925.403 -6.51; 933.449 -6.51; 941.565 -6.51; 949.752 -6.51; 958.01 -6.51; 966.339 -6.51; 974.741 -6.51; 983.216 -6.51; 991.765 -6.51; 1000.39 -6.396; 1009.09 -6.396; 1017.86 -6.396; 1026.71 -6.396; 1035.64 -6.396; 1044.64 -6.396; 1053.72 -6.396; 1062.89 -6.396; 1072.13 -6.396; 1081.45 -6.51; 1090.85 -6.51; 1100.34 -6.51; 1109.9 -6.51; 1119.55 -6.6239; 1129.29 -6.6239; 1139.11 -6.6239; 1149.01 -6.7379; 1159 -6.7379; 1169.08 -6.7379; 1179.24 -6.7379; 1189.5 -6.8519; 1199.84 -6.8519; 1210.27 -6.8519; 1220.79 -6.9658; 1231.41 -6.9658; 1242.11 -6.9658; 1252.91 -6.9658; 1263.81 -7.0798; 1274.8 -7.0798; 1285.88 -7.1937; 1297.06 -7.1937; 1308.34 -7.1937; 1319.71 -7.3077; 1331.19 -7.3077; 1342.76 -7.4217; 1354.44 -7.4217; 1366.21 -7.4217; 1378.09 -7.5356; 1390.07 -7.5356; 1402.16 -7.6496; 1414.35 -7.6496; 1426.65 -7.7635; 1439.05 -7.7635; 1451.56 -7.7635; 1464.19 -7.8775; 1476.92 -7.8775; 1489.76 -7.9915; 1502.71 -7.9915; 1515.78 -7.9915; 1528.95 -7.9915; 1542.25 -8.1054; 1555.66 -8.2194; 1569.18 -8.2194; 1582.83 -8.2194; 1596.59 -8.2194; 1610.47 -8.3333; 1624.47 -8.3333; 1638.6 -8.3333; 1652.84 -8.3333; 1667.22 -8.4473; 1681.71 -8.5613; 1696.33 -8.5613; 1711.08 -8.5613; 1725.96 -8.5613; 1740.97 -8.6752; 1756.1 -8.6752; 1771.37 -8.6752; 1786.77 -8.7892; 1802.31 -8.7892; 1817.98 -8.7892; 1833.79 -8.7892; 1849.73 -8.9031; 1865.81 -8.9031; 1882.04 -8.9031; 1898.4 -9.0171; 1914.9 -9.0171; 1931.55 -9.1311; 1948.35 -9.1311; 1965.29 -9.1311; 1982.38 -9.1311; 1999.61 -9.245; 2017 -9.245; 2034.54 -9.245; 2052.22 -9.359; 2070.07 -9.359; 2088.07 -9.4729; 2106.22 -9.4729; 2124.53 -9.5869; 2143.01 -9.5869; 2161.64 -9.5869; 2180.43 -9.7009; 2199.39 -9.7009; 2218.51 -9.7009; 2237.8 -9.7009; 2257.26 -9.8148; 2276.89 -9.8148; 2296.68 -9.9288; 2316.65 -9.9288; 2336.79 -9.9288; 2357.11 -10.0427; 2377.61 -10.0427; 2398.28 -10.1567; 2419.13 -10.1567; 2440.16 -10.1567; 2461.38 -10.2707; 2482.78 -10.2707; 2504.37 -10.2707; 2526.14 -10.2707; 2548.11 -10.3846; 2570.26 -10.3846; 2592.61 -10.4986; 2615.15 -10.4986; 2637.89 -10.4986; 2660.83 -10.6125; 2683.96 -10.6125; 2707.3 -10.6125; 2730.84 -10.6125; 2754.58 -10.7265; 2778.53 -10.7265; 2802.69 -10.7265; 2827.06 -10.8405; 2851.64 -10.8405; 2876.43 -10.8405; 2901.44 -10.8405; 2926.67 -10.8405; 2952.11 -10.9544; 2977.78 -10.9544; 3003.67 -10.9544; 3029.79 -10.9544; 3056.13 -10.9544; 3082.7 -10.9544; 3109.5 -11.0684; 3136.54 -11.0684; 3163.81 -11.0684; 3191.32 -11.0684; 3219.07 -11.0684; 3247.06 -11.0684; 3275.29 -11.0684; 3303.77 -11.0684; 3332.49 -11.0684; 3361.47 -11.0684; 3390.69 -11.0684; 3420.17 -11.0684; 3449.91 -11.0684; 3479.91 -11.0684; 3510.16 -11.0684; 3540.68 -11.0684; 3571.47 -11.1823; 3602.52 -11.1823; 3633.84 -11.1823; 3665.44 -11.1823; 3697.31 -11.1823; 3729.45 -11.1823; 3761.88 -11.1823; 3794.59 -11.1823; 3827.58 -11.1823; 3860.86 -11.1823; 3894.43 -11.1823; 3928.29 -11.1823; 3962.44 -11.0684; 3996.9 -11.0684; 4031.65 -11.0684; 4066.7 -10.9544; 4102.06 -10.9544; 4137.73 -10.8405; 4173.7 -10.8405; 4209.99 -10.7265; 4246.6 -10.6125; 4283.52 -10.4986; 4320.76 -10.3276; 4358.33 -10.1567; 4396.22 -9.9858; 4434.45 -9.9288; 4473 -9.7578; 4511.89 -9.5869; 4551.12 -9.359; 4590.69 -9.1311; 4630.61 -8.9031; 4670.87 -8.7322; 4711.48 -8.5043; 4752.45 -8.3903; 4793.77 -8.1624; 4835.45 -8.0484; 4877.49 -7.8205; 4919.9 -7.7635; 4962.68 -7.6496; 5005.82 -7.5356; 5049.35 -7.4217; 5093.25 -7.4217; 5137.53 -7.3077; 5182.2 -7.1937; 5227.26 -7.1937; 5272.71 -7.0798; 5318.55 -7.0798; 5364.8 -7.0798; 5411.44 -7.0798; 5458.49 -7.1937; 5505.95 -7.1937; 5553.82 -7.1937; 5602.11 -7.3077; 5650.82 -7.3077; 5699.95 -7.4217; 5749.51 -7.4217; 5799.5 -7.5356; 5849.93 -7.6496; 5900.79 -7.7635; 5952.1 -7.8775; 6003.85 -7.8775; 6056.05 -7.9915; 6108.7 -7.9915; 6161.82 -7.9915; 6215.39 -7.9915; 6269.43 -7.9915; 6323.94 -7.9915; 6378.93 -7.9915; 6434.39 -7.8775; 6490.33 -7.8775; 6546.76 -7.7635; 6603.69 -7.6496; 6661.1 -7.4786; 6719.02 -7.3077; 6777.44 -7.1937; 6836.37 -7.0798; 6895.81 -6.9658; 6955.76 -6.7949; 7016.24 -6.6239; 7077.24 -6.453; 7138.78 -6.2251; 7200.85 -6.1681; 7263.46 -6.0541; 7326.61 -5.9402; 7390.31 -5.8262; 7454.57 -5.7123; 7519.38 -5.5983; 7584.76 -5.4843; 7650.71 -5.3704; 7717.23 -5.2564; 7784.33 -5.1425; 7852.01 -5.0285; 7920.28 -5.0285; 7989.14 -4.91453; 8058.61 -4.91453; 8128.67 -4.80057; 8199.35 -4.80057; 8270.64 -4.80057; 8342.55 -4.80057; 8415.08 -4.80057; 8488.25 -4.80057; 8562.05 -4.80057; 8636.5 -4.91453; 8711.59 -5.0285; 8787.33 -5.0285; 8863.73 -5.1425; 8940.8 -5.2564; 9018.54 -5.3704; 9096.95 -5.4843; 9176.05 -5.7123; 9255.83 -5.9402; 9336.31 -6.2251; 9417.48 -6.51; 9499.36 -6.7379; 9581.96 -6.9658; 9665.27 -7.1937; 9749.3 -7.4217; 9834.07 -7.5926; 9919.57 -7.7635; 10005.8 -7.5926; 10092.8 -7.4786; 10180.6 -7.1368; 10269.1 -6.7949; 10358.4 -6.2821; 10448.4 -5.7692; 10539.3 -5.1994; 10630.9 -4.80057; 10723.4 -4.45869; 10816.6 -4.23077; 10910.6 -4.00285; 11005.5 -3.83191; 11101.2 -3.77493; 11197.7 -3.66097; 11295.1 -3.54701; 11393.3 -3.54701; 11492.3 -3.66097; 11592.3 -3.66097; 11693 -3.77493; 11794.7 -3.83191; 11897.3 -4.00285; 12000.7 -4.11681; 12105 -4.11681; 12210.3 -4.23077; 12316.5 -4.23077; 12423.5 -4.11681; 12531.6 -4.11681; 12640.5 -3.94587; 12750.4 -3.77493; 12861.3 -3.66097; 12973.1 -3.54701; 13085.9 -3.54701; 13199.7 -3.43305; 13314.5 -3.43305; 13430.2 -3.43305; 13547 -3.43305; 13664.8 -3.43305; 13783.6 -3.54701; 13903.4 -3.60399; 14024.3 -3.71795; 14146.3 -3.94587; 14269.2 -4.23077; 14393.3 -4.57265; 14518.5 -5.0285; 14644.7 -5.4843; 14772 -5.9402; 14900.5 -6.453; 15030 -6.9088; 15160.7 -7.3077; 15292.5 -7.1937; 15425.5 -6.6809; 15559.6 -5.9972; 15694.9 -5.3134; 15831.3 -4.57265; 15969 -3.94587; 16107.8 -3.26211; 16247.9 -2.63533; 16389.2 -2.06553; 16531.7 -1.55271; 16675.4 -1.15385; 16820.4 -0.86895; 16966.6 -0.58405; 17114.1 -0.35613; 17262.9 -0.12821; 17413 0.09972; 17564.4 0.32764; 17717.2 0.4416; 17871.2 0.55556; 18026.6 0.55556; 18183.3 0.55556; 18341.4 0.55556; 18500.9 0.38462; 18661.7 0.1567; 18824 -0.12821; 18987.7 -0.47009; 19152.8 -0.86895; 19319.3 -1.26781; 19487.3 -1.72365; 19656.7 -2.23647; 19827.6 -2.63533; 20000 -2.92023; 20173.9 -2.57835; 20349.3 -2.12251; 20526.2 -1.43875; 20704.7 -0.69801; 20884.7 0.21368; 21066.3 1.06838; 21249.5 1.80912; 21434.2 2.49288; 21620.6 2.94872; 21808.6 3.34758; 21998.2 3.68946; 22189.5 4.03134; 22382.4 4.31624; 22577 4.60114; 22773.3 4.82906; 22971.3 4.88604; 23171 5; 23372.5 5; 23780.7 4.88604");
	ArbitraryResponseEqualizerEnable(jdsp, 1);*/
	// FIR Equalizer
	double freq[NUMPTS] = { 25.0, 40.0, 63.0, 100.0, 160.0, 250.0, 400.0, 630.0, 1000.0, 1600.0, 2500.0, 4000.0, 6300.0, 10000.0, 16000.0 };
	double gains[NUMPTS] = { 5.0, -1.0, -4, -1, 2.1, 0.0, 0.0, 0.7, -10.7, 0.0, 0.0, 0.0, 0.0, 0.8, 8.0 };
	//double gains[NUMPTS] = { 0.0, 5.0, 0.0 };
	//MultimodalEqualizerAxisInterpolation(jdsp, 0, 0, freq, gains);
	MultimodalEqualizerAxisInterpolation(jdsp, 0, 5, freq, gains);
	MultimodalEqualizerEnable(jdsp, 1);
	JamesDSPSetSampleRate(jdsp, (float)sampleRate, 1);
	int readcount = (unsigned int)ceil((float)totalPCMFrameCount / (float)frameCountBest);
	int finalSize = frameCountBest * readcount;
	float *splittedBuffer[2];
	splittedBuffer[0] = (float *)calloc(finalSize, sizeof(float));
	splittedBuffer[1] = (float *)calloc(finalSize, sizeof(float));
	channel_splitFloat(pSampleData, (unsigned int)totalPCMFrameCount, splittedBuffer, channels);
	if (channels == 1)
		memcpy(splittedBuffer[1], splittedBuffer[0], finalSize * sizeof(float));
	free(pSampleData);
	void *dt1 = 0, *dt2 = 0;
	void *yt1 = 0, *yt2 = 0;
	const char simulateMultiplexing = 0;
	const char simulateDatatype = 4;
	if (simulateDatatype == 4) // Pack24
	{
		dt1 = malloc(frameCountBest * 3 * 2);
		dt2 = ((char *)dt1) + frameCountBest * 3;
		yt1 = malloc(frameCountBest * 3 * 2);
		yt2 = ((char *)yt1) + frameCountBest * 3;
	}
	else if (simulateDatatype == 3 || simulateDatatype == 2) // 8_24 / 32
	{
		dt1 = malloc(frameCountBest * sizeof(int32_t) * 2);
		dt2 = ((char *)dt1) + frameCountBest * sizeof(int32_t);
		yt1 = malloc(frameCountBest * sizeof(int32_t) * 2);
		yt2 = ((char *)yt1) + frameCountBest * sizeof(int32_t);
	}
	else if (simulateDatatype == 1) // 16
	{
		dt1 = malloc(frameCountBest * sizeof(int16_t) * 2);
		dt2 = ((char *)dt1) + frameCountBest * sizeof(int16_t);
		yt1 = malloc(frameCountBest * sizeof(int16_t) * 2);
		yt2 = ((char *)yt1) + frameCountBest * sizeof(int16_t);
	}
	else if (simulateDatatype == 0) // float
	{
		dt1 = malloc(frameCountBest * sizeof(float) * 2);
		dt2 = ((char *)dt1) + frameCountBest * sizeof(float);
		yt1 = malloc(frameCountBest * sizeof(float) * 2);
		yt2 = ((char *)yt1) + frameCountBest * sizeof(float);
	}
	int targetChannels = 2;
	float **outBuffer = (float **)malloc(targetChannels * sizeof(float *));
	for (i = 0; i < targetChannels; i++)
		outBuffer[i] = (float *)calloc(finalSize, sizeof(float));
	printf("[Info] Processing...\n");
	pthread_t inc_x_thread;
	threadTest dataThread;
	dataThread.dspPtr = jdsp;
	readCountN = readcount;
	//pthread_create(&inc_x_thread, NULL, inc_eq, &dataThread);
	for (i = 0; i < readcount; i++)
	{
		idxN = i;
		unsigned int pointerOffset = frameCountBest * i;
		float *ptrInLeft = splittedBuffer[0] + pointerOffset;
		float *ptrInRight = splittedBuffer[1] + pointerOffset;
		float *outl = outBuffer[0] + pointerOffset;
		float *outr = outBuffer[1] + pointerOffset;
		if (i == 400)
		{
			//CompressorSetParam(jdsp, 0.8, 4, 2, 1);
			//pthread_create(&inc_x_thread, NULL, inc_x, &dataThread);
			//ArbitraryResponseEqualizerStringParser(jdsp, "cce2: 5000.0 3.0; 2000.0 0.0; 0.0 -100.0; 100.0 -20, 400.0; 0;");
		}
		if (i > 50 && i < 400)
		{
			gains[5] += 0.09;
			MultimodalEqualizerAxisInterpolation(jdsp, 0, 0, freq, gains);
		}
		else if (i >= 400)
		{
			gains[5] -= 0.3;
			gains[10] -= 0.1;
			MultimodalEqualizerAxisInterpolation(jdsp, 0, 0, freq, gains);
		}
		/*if (i == 200)
		{
			text = openTextFile("stftCentreCut.eel");
			if (text)
			{
				LiveProgStringParser(jdsp, text);
				free(text);
			}
		}
		if (i == 300)
		{
			text = openTextFile("fftConvolutionHRTF1.eel");
			if (text)
			{
				LiveProgStringParser(jdsp, text);
				free(text);
			}
		}
		if (i == 600)
		{
			text = openTextFile("autopeakfilter.eel");
			if (text)
			{
				LiveProgStringParser(jdsp, text);
				free(text);
			}
		}
		if (i == 800)
		{
			text = openTextFile("stftDenoise.eel");
			if (text)
			{
				LiveProgStringParser(jdsp, text);
				free(text);
			}
		}
		if (i == 1000)
		{
			text = openTextFile("fftConvolution2x4x2.eel");
			if (text)
			{
				LiveProgStringParser(jdsp, text);
				free(text);
			}
		}*/
		if (!simulateMultiplexing)
		{
			if (simulateDatatype == 4)
			{
				uint8_t *casted1 = dt1;
				uint8_t *casted2 = dt2;
				uint8_t *ycasted1 = yt1;
				uint8_t *ycasted2 = yt2;
				static const float scale = 1.0f / (float)(1UL << 31);
				for (int j = 0; j < frameCountBest; j++)
				{
					jdsp->p24_from_i32(clamp24_from_float(ptrInLeft[j]), casted1 + j * 3);
					jdsp->p24_from_i32(clamp24_from_float(ptrInRight[j]), casted2 + j * 3);
				}
				jdsp->processInt24PackedDeinterleaved(jdsp, casted1, casted2, ycasted1, ycasted2, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)jdsp->i32_from_p24(ycasted1 + j * 3) * scale;
					outr[j] = (float)jdsp->i32_from_p24(ycasted2 + j * 3) * scale;
				}
			}
			else if (simulateDatatype == 3 || simulateDatatype == 2)
			{
				int32_t *casted1 = dt1;
				int32_t *casted2 = dt2;
				int32_t *ycasted1 = yt1;
				int32_t *ycasted2 = yt2;
				float scale;
				if (simulateDatatype == 3)
					scale = 1.0 / (float)(1 << 23);
				else
					scale = 1.0 / (float)(1 << 31);
				for (int j = 0; j < frameCountBest; j++)
				{
					casted1[j] = ptrInLeft[j] / scale;
					casted2[j] = ptrInRight[j] / scale;
				}
				if (simulateDatatype == 3)
					jdsp->processInt8_24Deinterleaved(jdsp, casted1, casted2, ycasted1, ycasted2, frameCountBest);
				else
					jdsp->processInt32Deinterleaved(jdsp, casted1, casted2, ycasted1, ycasted2, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)ycasted1[j] * scale;
					outr[j] = (float)ycasted2[j] * scale;
				}
			}
			else if (simulateDatatype == 1)
			{
				int16_t *casted1 = dt1;
				int16_t *casted2 = dt2;
				int16_t *ycasted1 = yt1;
				int16_t *ycasted2 = yt2;
				float scale;
				scale = 1.0 / (float)(1 << 15);
				for (int j = 0; j < frameCountBest; j++)
				{
					casted1[j] = ptrInLeft[j] / scale;
					casted2[j] = ptrInRight[j] / scale;
				}
				jdsp->processInt16Deinterleaved(jdsp, casted1, casted2, ycasted1, ycasted2, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)ycasted1[j] * scale;
					outr[j] = (float)ycasted2[j] * scale;
				}
			}
			else
				jdsp->processFloatDeinterleaved(jdsp, ptrInLeft, ptrInRight, outl, outr, frameCountBest);
		}
		else
		{
			if (simulateDatatype == 4)
			{
				uint8_t *casted = dt1;
				uint8_t *ycasted = yt1;
				static const float scale = 1.0f / (float)(1UL << 31);
				for (int j = 0; j < frameCountBest; j++)
				{
					jdsp->p24_from_i32(clamp24_from_float(ptrInLeft[j]), casted + (j << 1) * 3);
					jdsp->p24_from_i32(clamp24_from_float(ptrInRight[j]), casted + ((j << 1) + 1) * 3);
				}
				jdsp->processInt24PackedMultiplexd(jdsp, casted, ycasted, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)jdsp->i32_from_p24(ycasted + (j << 1) * 3) * scale;
					outr[j] = (float)jdsp->i32_from_p24(ycasted + ((j << 1) + 1) * 3) * scale;
				}
			}
			else if (simulateDatatype == 3 || simulateDatatype == 2)
			{
				int32_t *casted = dt1;
				int32_t *ycasted = yt1;
				float scale;
				if (simulateDatatype == 3)
					scale = 1.0 / (float)(1 << 23);
				else
					scale = 1.0 / (float)(1 << 31);
				for (int j = 0; j < frameCountBest; j++)
				{
					casted[j << 1] = ptrInLeft[j] / scale;
					casted[(j << 1) + 1] = ptrInRight[j] / scale;
				}
				if (simulateDatatype == 3)
					jdsp->processInt8_24Multiplexd(jdsp, casted, ycasted, frameCountBest);
				else
					jdsp->processInt32Multiplexd(jdsp, casted, ycasted, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)ycasted[j << 1] * scale;
					outr[j] = (float)ycasted[(j << 1) + 1] * scale;
				}
			}
			else if (simulateDatatype == 1)
			{
				int16_t *casted = dt1;
				int16_t *ycasted = yt1;
				float scale;
				scale = 1.0 / (float)(1 << 15);
				for (int j = 0; j < frameCountBest; j++)
				{
					casted[j << 1] = ptrInLeft[j] / scale;
					casted[(j << 1) + 1] = ptrInRight[j] / scale;
				}
				jdsp->processInt16Multiplexd(jdsp, casted, ycasted, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = (float)ycasted[j << 1] * scale;
					outr[j] = (float)ycasted[(j << 1) + 1] * scale;
				}
			}
			else
			{
				float *casted = dt1;
				float *ycasted = yt1;
				for (int j = 0; j < frameCountBest; j++)
				{
					casted[j << 1] = ptrInLeft[j];
					casted[(j << 1) + 1] = ptrInRight[j];
				}
				jdsp->processFloatMultiplexd(jdsp, casted, ycasted, frameCountBest);
				for (int j = 0; j < frameCountBest; j++)
				{
					outl[j] = ycasted[j << 1];
					outr[j] = ycasted[(j << 1) + 1];
				}
			}
		}
	}
	free(dt1);
	free(yt1);
	JamesDSPFree(jdsp);
	JamesDSPGlobalMemoryDeallocation();
	free(jdsp);
	free(splittedBuffer[0]);
	free(splittedBuffer[1]);
	unsigned int totalFrames = finalSize * targetChannels;
	float *sndBuf = (float *)calloc(totalFrames, sizeof(float));
	channel_joinFloat(outBuffer, targetChannels, sndBuf, finalSize);
	for (i = 0; i < targetChannels; i++)
		free(outBuffer[i]);
	free(outBuffer);
	size_t bufsz = snprintf(NULL, 0, "%s_Processed.wav", filename);
	char *filenameNew = (char *)malloc(bufsz + 1);
	snprintf(filenameNew, bufsz + 1, "%s_Processed.wav", filename);
	free(filename);
	SetCurrentDirectory(oldPath);
	drwav pWav;
	drwav_data_format format;
	format.container = drwav_container_riff;
	format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
	format.channels = targetChannels;
	format.sampleRate = sampleRate;
	format.bitsPerSample = 32;
	unsigned int fail = drwav_init_file_write(&pWav, filenameNew, &format, 0);
	drwav_uint64 framesWritten = drwav_write_pcm_frames(&pWav, totalPCMFrameCount, sndBuf);
	drwav_uninit(&pWav);
	free(filenameNew);
	free(sndBuf);
	return 0;
}