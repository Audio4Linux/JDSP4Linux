#define TAG "BassBoost"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#include "EffectBassBoost.h"
typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t data;
} reply1x4_1x4_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data;
} reply1x4_1x2_t;

EffectBassBoost::EffectBassBoost()
    : mStrength(0), mFilterType(0), mCenterFrequency(55.0f)
{
}

int32_t EffectBassBoost::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            int32_t *replyData = (int32_t *) pReplyData;
            *replyData = ret;
            return 0;
        }

        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_GET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == BASSBOOST_PARAM_STRENGTH_SUPPORTED) {
                reply1x4_1x4_t *replyData = (reply1x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = 1;
                *replySize = sizeof(reply1x4_1x4_t);
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_STRENGTH) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mStrength;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_FILTER_SLOPE) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mFilterType;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_CENTER_FREQUENCY) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mCenterFrequency;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
        }

        effect_param_t *replyData = (effect_param_t *) pReplyData;
        replyData->status = -EINVAL;
        replyData->vsize = 0;
        *replySize = sizeof(effect_param_t);
        return 0;
    }

    if (cmdCode == EFFECT_CMD_SET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == BASSBOOST_PARAM_STRENGTH) {
                mStrength = ((int16_t *) cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_FILTER_SLOPE) {
                mFilterType = ((int16_t* )cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_CENTER_FREQUENCY) {
                mCenterFrequency = ((int16_t* )cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
        }
        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = -EINVAL;
        return 0;
    }

    return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}

void EffectBassBoost::refreshStrength()
{
    if(mFilterType == 0)
    {
	const int order = 1;
	float gain = mStrength / 60;
	Iir::Butterworth::LowShelf<order> f;
	f.setup (order, mSamplingRate, mCenterFrequency, gain);
	f.reset();
	int n = f.getNumStages();
	LOGI("NumStages = %d", n);
	for(int i=0; i<n; i++)
	{
		 LOGI("SOS %d: A0 = %9.50f", i, f[i].getA0());
		 LOGI("SOS %d: A1 = %9.50f", i, f[i].getA1());
		 LOGI("SOS %d: A2 = %9.50f", i, f[i].getA2());
		 LOGI("SOS %d: B0 = %9.50f", i, f[i].getB0());
		 LOGI("SOS %d: B1 = %9.50f", i, f[i].getB1());
		 LOGI("SOS %d: B2 = %9.50f", i, f[i].getB2());
	}
    mBoostL.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
    mBoostR.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
    }
    else if(mFilterType == 1)
    {
	const int order = 6;
	float gain = mStrength / 60;
	Iir::Butterworth::LowShelf<order> f;
	f.setup (order, mSamplingRate, mCenterFrequency, gain);
	f.reset();
	int n = f.getNumStages();
	LOGI("NumStages = %d", n);
	for(int i=0; i<n; i++)
	{
		 LOGI("SOS %d: A0 = %9.50f", i, f[i].getA0());
		 LOGI("SOS %d: A1 = %9.50f", i, f[i].getA1());
		 LOGI("SOS %d: A2 = %9.50f", i, f[i].getA2());
		 LOGI("SOS %d: B0 = %9.50f", i, f[i].getB0());
		 LOGI("SOS %d: B1 = %9.50f", i, f[i].getB1());
		 LOGI("SOS %d: B2 = %9.50f", i, f[i].getB2());
	}
    	    mStage1L.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
    	    mStage1R.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
    	    mStage2L.setSOS(0, f[1].getA0(), f[1].getA1(), f[1].getA2(), f[1].getB0(), f[1].getB1(), f[1].getB2());
    	    mStage2R.setSOS(0, f[1].getA0(), f[1].getA1(), f[1].getA2(), f[1].getB0(), f[1].getB1(), f[1].getB2());
    	    mBoostL.setSOS(0, f[2].getA0(), f[2].getA1(), f[2].getA2(), f[2].getB0(), f[2].getB1(), f[2].getB2());
    	    mBoostR.setSOS(0, f[2].getA0(), f[2].getA1(), f[2].getA2(), f[2].getB0(), f[2].getB1(), f[2].getB2());
    	}
    	else
    	{
	const int order = 1;
	float gain = mStrength / 60;
	Iir::Butterworth::LowShelf<order> f;
	f.setup (order, mSamplingRate, mCenterFrequency, gain);
	f.reset();
	int n = f.getNumStages();
	LOGI("NumStages = %d", n);
	for(int i=0; i<n; i++)
	{
		 LOGI("SOS %d: A0 = %9.50f", i, f[i].getA0());
		 LOGI("SOS %d: A1 = %9.50f", i, f[i].getA1());
		 LOGI("SOS %d: A2 = %9.50f", i, f[i].getA2());
		 LOGI("SOS %d: B0 = %9.50f", i, f[i].getB0());
		 LOGI("SOS %d: B1 = %9.50f", i, f[i].getB1());
		 LOGI("SOS %d: B2 = %9.50f", i, f[i].getB2());
	}
    mBoostL.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
    mBoostR.setSOS(0, f[0].getA0(), f[0].getA1(), f[0].getA2(), f[0].getB0(), f[0].getB1(), f[0].getB2());
	}
}

int32_t EffectBassBoost::process(audio_buffer_t* in, audio_buffer_t* out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
    	if(mFilterType == 0)
    	{
       	int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
    	int32_t boostl = mBoostL.process(dryL);
    	int32_t boostr = mBoostR.process(dryR);
        write(out, i * 2, boostl);
        write(out, i * 2 + 1, boostr);
    	}
    	else if(mFilterType == 1)
    	{
       	int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
    	int32_t boostl = mStage1L.process(dryL);
    	int32_t boostr = mStage1R.process(dryR);
    	int32_t boostl2st = mStage2L.process(boostl);
    	int32_t boostr2st = mStage2R.process(boostr);
    	int32_t comboostl = mBoostL.process(boostl2st);
    	int32_t comboostr = mBoostR.process(boostr2st);
        write(out, i * 2, comboostl);
        write(out, i * 2 + 1, comboostr);
    	}
    	else
    	{
       	int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
    	int32_t boostl = mBoostL.process(dryL);
    	int32_t boostr = mBoostR.process(dryR);
        write(out, i * 2, boostl);
        write(out, i * 2 + 1, boostr);
	}
}
    return mEnable ? 0 : -ENODATA;
}
