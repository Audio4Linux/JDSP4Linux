#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../libsndfile\sndfile.h"
#include "../libsamplerate\samplerate.h"
#include "AutoConvolver.h"
SF_INFO sfiIRInfo;
SNDFILE *sfIRFile;
JNIEXPORT jintArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_GetLoadImpulseResponseInfo
(JNIEnv *env, jobject obj, jstring path)
{
	const char *mIRFileName = (*env)->GetStringUTFChars(env, path, 0);
	if (strlen(mIRFileName) <= 0) return 0;
	jint jImpInfo[4] = { 0, 0, 0, 0 };
	jintArray jrImpInfo = (*env)->NewIntArray(env, 4);
	if (!jrImpInfo) return 0;
	memset(&sfiIRInfo, 0, sizeof(SF_INFO));
	sfIRFile = sf_open(mIRFileName, SFM_READ, &sfiIRInfo);
	if (sfIRFile == NULL)
	{
		// Open failed or invalid wave file
		return 0;
	}
	// Sanity check
	if ((sfiIRInfo.channels != 1) && (sfiIRInfo.channels != 2) && (sfiIRInfo.channels != 4))
	{
		sf_close(sfIRFile);
		return 0;
	}
	if ((sfiIRInfo.samplerate <= 0) || (sfiIRInfo.frames <= 0))
	{
		// Negative sampling rate or empty data ?
		sf_close(sfIRFile);
		return 0;
	}
	jImpInfo[0] = (jint)sfiIRInfo.channels;
	jImpInfo[1] = (jint)sfiIRInfo.frames;
	jImpInfo[2] = (jint)sfiIRInfo.samplerate;
	jImpInfo[3] = (jint)sfiIRInfo.format;
	(*env)->SetIntArrayRegion(env, jrImpInfo, 0, 4, jImpInfo);
	(*env)->ReleaseStringUTFChars(env, path, mIRFileName);
	return jrImpInfo;
}
JNIEXPORT jfloatArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_ReadImpulseResponseToFloat
(JNIEnv *env, jobject obj, jint targetSampleRate)
{
	// Allocate memory block for reading
	jfloatArray outbuf;
	int i;
	float *final;
	int frameCountTotal = sfiIRInfo.channels * sfiIRInfo.frames;
	size_t bufferSize = frameCountTotal * sizeof(float);
	float *pFrameBuffer = (float*)malloc(bufferSize);
	if (!pFrameBuffer)
	{
		// Memory not enough
		sf_close(sfIRFile);
		return 0;
	}
	sf_readf_float(sfIRFile, pFrameBuffer, sfiIRInfo.frames);
	sf_close(sfIRFile);
	if (sfiIRInfo.samplerate == targetSampleRate)
	{
		final = (float*)malloc(bufferSize);
		memcpy(final, pFrameBuffer, bufferSize);
		// Prepare return array
		outbuf = (*env)->NewFloatArray(env, (jsize)frameCountTotal);
		(*env)->SetFloatArrayRegion(env, outbuf, 0, (jsize)frameCountTotal, final);
		free(final);
	}
	else
	{
		double convertionRatio = (double)targetSampleRate / (double)sfiIRInfo.samplerate;
		int resampledframeCountTotal = (int)((double)frameCountTotal * convertionRatio);
		int outFramesPerChannel = (int)((double)sfiIRInfo.frames * convertionRatio);
		bufferSize = resampledframeCountTotal * sizeof(float);
		float *out = (float*)malloc(bufferSize);
		int error;
		SRC_DATA data;
		data.data_in = pFrameBuffer;
		data.data_out = out;
		data.input_frames = sfiIRInfo.frames;
		data.output_frames = outFramesPerChannel;
		data.src_ratio = convertionRatio;
		error = src_simple(&data, 1, sfiIRInfo.channels);
		final = (float*)malloc(bufferSize);
		memcpy(final, out, bufferSize);
		jsize jsFrameBufferSize = (unsigned int)resampledframeCountTotal;
		outbuf = (*env)->NewFloatArray(env, jsFrameBufferSize);
		(*env)->SetFloatArrayRegion(env, outbuf, 0, jsFrameBufferSize, final);
		free(out);
		free(final);
	}
	free(pFrameBuffer);
	return outbuf;
}
JNIEXPORT jstring JNICALL Java_james_dsp_activity_JdspImpResToolbox_OfflineAudioResample
(JNIEnv *env, jobject obj, jstring path, jstring filename, jint targetSampleRate)
{
	SF_INFO sfinfo;
	SNDFILE *infile, *outfile;
	sf_count_t count;
	const char *jnipath = (*env)->GetStringUTFChars(env, path, 0);
	if (strlen(jnipath) <= 0) return 0;
	const char *mIRFileName = (*env)->GetStringUTFChars(env, filename, 0);
	if (strlen(mIRFileName) <= 0) return 0;
	memset(&sfinfo, 0, sizeof(SF_INFO));
	size_t needed = snprintf(NULL, 0, "%s%s", jnipath, mIRFileName) + 1;
	char *filenameIR = malloc(needed);
	snprintf(filenameIR, needed, "%s%s", jnipath, mIRFileName);
	infile = sf_open(filenameIR, SFM_READ, &sfinfo);
	free(filenameIR);
	if (infile == NULL)
	{
		// Open failed or invalid wave file
		return NULL;
	}
	if ((sfinfo.channels != 1) && (sfinfo.channels != 2) && (sfinfo.channels != 4))
	{
		sf_close(infile);
		return NULL;
	}
	if ((sfinfo.samplerate <= 0) || (sfinfo.frames <= 0))
	{
		// Negative sampling rate or empty data ?
		sf_close(infile);
		return NULL;
	}
	double src_ratio = (double)targetSampleRate / (double)sfinfo.samplerate;
	needed = snprintf(NULL, 0, "%s%d_%s", jnipath, targetSampleRate, mIRFileName) + 1;
	filenameIR = malloc(needed);
	snprintf(filenameIR, needed, "%s%d_%s", jnipath, targetSampleRate, mIRFileName);
	//
	int frameCountTotal = sfinfo.channels * sfinfo.frames;
	int resampledframeCountTotal = (int)((double)frameCountTotal * src_ratio);
	int outFramesPerChannel = (int)((double)sfinfo.frames * src_ratio);
	float *pFrameBuffer = (float*)malloc(frameCountTotal * sizeof(float));
	if (!pFrameBuffer)
	{
		// Memory not enough
		sf_close(infile);
		return NULL;
	}
	sf_readf_float(infile, pFrameBuffer, sfinfo.frames);
	sf_close(infile);
	float *out = (float*)calloc(resampledframeCountTotal, sizeof(float));
	int error;
	SRC_DATA data;
	data.data_in = pFrameBuffer;
	data.data_out = out;
	data.input_frames = sfinfo.frames;
	data.output_frames = outFramesPerChannel;
	data.src_ratio = src_ratio;
	error = src_simple(&data, 0, sfinfo.channels);
	free(pFrameBuffer);
	sfinfo.frames = outFramesPerChannel;
	sfinfo.samplerate = targetSampleRate;
	outfile = sf_open(filenameIR, SFM_WRITE, &sfinfo);
	sf_writef_float(outfile, out, outFramesPerChannel);
	sf_close(outfile);
	free(out);
	(*env)->ReleaseStringUTFChars(env, path, jnipath);
	(*env)->ReleaseStringUTFChars(env, filename, mIRFileName);
	jstring finalName = (*env)->NewStringUTF(env, filenameIR);
	free(filenameIR);
	return finalName;
}
JNIEXPORT jint JNICALL Java_james_dsp_activity_JdspImpResToolbox_FFTConvolutionBenchmark
(JNIEnv *env, jobject obj, jint entriesGen, jint fs, jdoubleArray c0, jdoubleArray c1)
{
	double **result = PartitionHelperDirect(entriesGen, fs);
	jdouble *bufc0 = (*env)->GetDoubleArrayElements(env, c0, 0);
	jdouble *bufc1 = (*env)->GetDoubleArrayElements(env, c1, 0);
	int length = (int)(*env)->GetArrayLength(env, c0);
	if (length < entriesGen)
		entriesGen = length;
	for (int i = 0; i < entriesGen; i++)
	{
		bufc0[i] = result[0][i];
		bufc1[i] = result[1][i];
	}
	free(result[0]);
	free(result[1]);
	free(result);
	(*env)->ReleaseDoubleArrayElements(env, c0, bufc0, 0);
	(*env)->ReleaseDoubleArrayElements(env, c1, bufc1, 0);
	return entriesGen;
}
JNIEXPORT jstring JNICALL Java_james_dsp_activity_JdspImpResToolbox_FFTConvolutionBenchmarkToString
(JNIEnv *env, jobject obj, jint entriesGen, jint fs)
{
	char *result = PartitionHelper(entriesGen, fs);
	jstring finalResult = (*env)->NewStringUTF(env, result);
	free(result);
	return finalResult;
}
