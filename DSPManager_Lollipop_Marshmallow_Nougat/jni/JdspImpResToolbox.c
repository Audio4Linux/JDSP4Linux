#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libsndfile\sndfile.h"
SF_INFO sfiIRInfo;
SNDFILE *sfIRFile;
JNIEXPORT jintArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_GetLoadImpulseResponseInfo
(JNIEnv *env, jobject obj, jstring path)
{
    const char *mIRFileName = (*env)->GetStringUTFChars(env, path, 0);
    if (strlen(mIRFileName) <= 0) return NULL;
    jint jImpInfo[4] = {0, 0, 0, 0};
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
    if ((sfiIRInfo.channels != 1) && (sfiIRInfo.channels != 2))
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
    jImpInfo[0] = (jint) sfiIRInfo.channels;
    jImpInfo[1] = (jint) sfiIRInfo.frames;
    jImpInfo[2] = (jint) sfiIRInfo.samplerate;
    jImpInfo[3] = (jint) sfiIRInfo.format;
    (*env)->SetIntArrayRegion(env, jrImpInfo, 0, 4, jImpInfo);
    return jrImpInfo;
}
JNIEXPORT jintArray JNICALL Java_james_dsp_activity_JdspImpResToolbox_ReadImpulseResponseToInt
(JNIEnv *env, jobject obj)
{
    // Allocate memory block for reading
    int *pFrameBuffer = (int*)malloc(sfiIRInfo.channels * sfiIRInfo.frames * sizeof(int));
    if (pFrameBuffer == NULL)
    {
        // Memory not enough
        sf_close(sfIRFile);
        return NULL;
    }
    sf_readf_int(sfIRFile, pFrameBuffer, sfiIRInfo.frames);
    sf_close(sfIRFile);
    // Prepare return array
    jsize jsFrameBufferSize = sfiIRInfo.frames * sfiIRInfo.channels;
    jintArray outbuf = (*env)->NewIntArray(env, jsFrameBufferSize);
    (*env)->SetIntArrayRegion(env, outbuf, 0, jsFrameBufferSize, pFrameBuffer);
    free(pFrameBuffer);
    return outbuf;
}
