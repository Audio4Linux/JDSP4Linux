#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libsndfile\sndfile.h"
#include "libsamplerate\samplerate.h"
SF_INFO sfiIRInfo;
SNDFILE *sfIRFile;
double apply_gain(float * data, long frames, int channels, double max, double gain)
{
	long k;
	for (k = 0; k < frames * channels; k++)
	{
		data[k] *= gain;

		if (fabs(data[k]) > max)
			max = fabs(data[k]);
	};
	return max;
} /* apply_gain */
sf_count_t sample_rate_convert(SNDFILE *infile, SNDFILE *outfile, int converter, double src_ratio, int channels, double * gain)
{
	float input[4096];
	float output[4096];

	SRC_STATE	*src_state;
	SRC_DATA	src_data;
	int			error;
	double		max = 0.0;
	sf_count_t	output_count = 0;

	sf_seek(infile, 0, SEEK_SET);
	sf_seek(outfile, 0, SEEK_SET);

	/* Initialize the sample rate converter. */
	if ((src_state = src_new(converter, channels, &error)) == NULL)
	{
		return 0;
	};

	src_data.end_of_input = 0; /* Set this later. */

							   /* Start with zero to force load in while loop. */
	src_data.input_frames = 0;
	src_data.data_in = input;

	src_data.src_ratio = src_ratio;

	src_data.data_out = output;
	src_data.output_frames = 4096 / channels;

	while (1)
	{
		/* If the input buffer is empty, refill it. */
		if (src_data.input_frames == 0)
		{
			src_data.input_frames = sf_readf_float(infile, input, 4096 / channels);
			src_data.data_in = input;

			/* The last read will not be a full buffer, so snd_of_input. */
			if (src_data.input_frames < 4096 / channels)
				src_data.end_of_input = SF_TRUE;
		};

		if ((error = src_process(src_state, &src_data)))
		{
			return 0;
		};

		/* Terminate if done. */
		if (src_data.end_of_input && src_data.output_frames_gen == 0)
			break;

		max = apply_gain(src_data.data_out, src_data.output_frames_gen, channels, max, *gain);

		/* Write output. */
		sf_writef_float(outfile, output, src_data.output_frames_gen);
		output_count += src_data.output_frames_gen;

		src_data.data_in += src_data.input_frames_used * channels;
		src_data.input_frames -= src_data.input_frames_used;
	};
	src_delete(src_state);
	return output_count;
} /* sample_rate_convert */
JNIEXPORT jintArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_GetLoadImpulseResponseInfo
(JNIEnv *env, jobject obj, jstring path)
{
	const char *mIRFileName = (*env)->GetStringUTFChars(env, path, 0);
	if (strlen(mIRFileName) <= 0) return NULL;
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
		// Convolver supports mono or stereo ir for now
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
JNIEXPORT jintArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_ReadImpulseResponseToInt
(JNIEnv *env, jobject obj, jint targetSampleRate)
{
	// Allocate memory block for reading
	jintArray outbuf;
	if (sfiIRInfo.samplerate == targetSampleRate)
	{
		int *pFrameBuffer = (int*)malloc(sfiIRInfo.channels * sfiIRInfo.frames * sizeof(int));
		if (!pFrameBuffer)
		{
			// Memory not enough
			sf_close(sfIRFile);
			return 0;
		}
		sf_readf_int(sfIRFile, pFrameBuffer, sfiIRInfo.frames);
		sf_close(sfIRFile);
		// Prepare return array
		jsize jsFrameBufferSize = sfiIRInfo.frames * sfiIRInfo.channels;
		outbuf = (*env)->NewIntArray(env, jsFrameBufferSize);
		(*env)->SetIntArrayRegion(env, outbuf, 0, jsFrameBufferSize, pFrameBuffer);
		free(pFrameBuffer);
	}
	else
	{
		double convertionRatio = (double)targetSampleRate / (double)sfiIRInfo.samplerate;
		int frameCount = sfiIRInfo.channels * sfiIRInfo.frames;
		int resampledframeCount = (int)((double)frameCount * convertionRatio + 0.5);
		float *pFrameBuffer = (float*)malloc(frameCount * sizeof(float));
		if (!pFrameBuffer)
		{
			// Memory not enough
			sf_close(sfIRFile);
			return 0;
		}
		sf_readf_float(sfIRFile, pFrameBuffer, sfiIRInfo.frames);
		sf_close(sfIRFile);
		float *out = (float*)malloc(resampledframeCount * sizeof(float)); /* Allocate output buf. */
		int error, i = 0;
		SRC_DATA data;
		data.data_in = pFrameBuffer;
		data.data_out = out;
		data.input_frames = sfiIRInfo.frames;
		data.output_frames = resampledframeCount;
		data.src_ratio = convertionRatio;
		error = src_simple(&data, 3, sfiIRInfo.channels);
		int finalOut = data.output_frames_gen * sfiIRInfo.channels;
		int *final = (int*)malloc(finalOut * sizeof(int));
		for (int i = 0; i < finalOut; i++)
			final[i] = out[i] * 32768.0f;
		jsize jsFrameBufferSize = finalOut;
		outbuf = (*env)->NewIntArray(env, jsFrameBufferSize);
		(*env)->SetIntArrayRegion(env, outbuf, 0, jsFrameBufferSize, final);
		free(pFrameBuffer);
		free(out);
		free(final);
	}
	return outbuf;
}
JNIEXPORT jint JNICALL Java_james_dsp_activity_JdspImpResToolbox_OfflineAudioResample
(JNIEnv *env, jobject obj, jstring path, jstring filename, jint targetSampleRate)
{
	SF_INFO sfinfo;
	SNDFILE *infile, *outfile;
	sf_count_t count;
	const char *jnipath = (*env)->GetStringUTFChars(env, path, 0);
	if (strlen(jnipath) <= 0) return NULL;
	const char *mIRFileName = (*env)->GetStringUTFChars(env, filename, 0);
	if (strlen(mIRFileName) <= 0) return NULL;
	memset(&sfinfo, 0, sizeof(SF_INFO));
	size_t needed = snprintf(NULL, 0, "%s%s", jnipath, mIRFileName) + 1;
	char *filenameIR = malloc(needed);
	snprintf(filenameIR, needed, "%s%s", jnipath, mIRFileName);
	infile = sf_open(filenameIR, SFM_READ, &sfinfo);
	free(filenameIR);
	if (infile == NULL)
	{
		// Open failed or invalid wave file
		return 0;
	}
	// Sanity check
	if ((sfinfo.channels != 1) && (sfinfo.channels != 2) && (sfinfo.channels != 4))
	{
		// Convolver supports mono or stereo ir for now
		sf_close(infile);
		return 0;
	}
	if ((sfinfo.samplerate <= 0) || (sfinfo.frames <= 0))
	{
		// Negative sampling rate or empty data ?
		sf_close(infile);
		return 0;
	}
	double gain = 1.0;
	double src_ratio = (double)targetSampleRate / (double)sfinfo.samplerate;
	sfinfo.samplerate = targetSampleRate;
	needed = snprintf(NULL, 0, "%s%d_%s", jnipath, targetSampleRate, mIRFileName) + 1;
	filenameIR = malloc(needed);
	snprintf(filenameIR, needed, "%s%d_%s", jnipath, targetSampleRate, mIRFileName);
	do
	{
		if ((outfile = sf_open(filenameIR, SFM_WRITE, &sfinfo)) == NULL)
		{
			sf_close(infile);
			return 0;
		};
		count = sample_rate_convert(infile, outfile, 0, src_ratio, sfinfo.channels, &gain);
	} while (count < 0);
	sf_close(infile);
	sf_close(outfile);
	free(filenameIR);
	(*env)->ReleaseStringUTFChars(env, path, jnipath);
	(*env)->ReleaseStringUTFChars(env, filename, mIRFileName);
	return count;
}
