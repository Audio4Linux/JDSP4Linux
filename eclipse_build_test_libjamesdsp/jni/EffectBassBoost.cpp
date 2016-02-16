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
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_FILTER_SLOPE) {
                mFilterType = ((int16_t* )cep)[8];
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_CENTER_FREQUENCY) {
                mCenterFrequency = ((int16_t* )cep)[8];
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

int32_t EffectBassBoost::process(audio_buffer_t* in, audio_buffer_t* out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
    	if(mFilterType == 0)
    	{
/*    	    mBoost.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
    	    mBoostL.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
    	    mBoostR.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f); */
        	int32_t dryL = read(in, i * 2);
            int32_t dryR = read(in, i * 2 + 1);
            const int order = 2;
        	Iir::Butterworth::LowShelf<order> f;
        	f.setup(order, 44100.0, mCenterFrequency, mStrength/60.);
        	f.reset();
        	int32_t boostl = f.filter(dryL);
        	int32_t boostr = f.filter(dryR);
            write(out, i * 2, boostl);
            write(out, i * 2 + 1, boostr);
    	}
    	else if(mFilterType == 1)
    	{
        	int32_t dryL = read(in, i * 2);
            int32_t dryR = read(in, i * 2 + 1);
            const int order = 4;
        	Iir::Butterworth::LowShelf<order> f;
        	f.setup(order, 44100.0, mCenterFrequency, mStrength/60.);
        	f.reset();
        	int32_t boostl = f.filter(dryL);
        	int32_t boostr = f.filter(dryR);
        write(out, i * 2, boostl);
        write(out, i * 2 + 1, boostr);
    	}
    	else
    	{
        	int32_t dryL = read(in, i * 2);
            int32_t dryR = read(in, i * 2 + 1);
            const int order = 2;
        	Iir::Butterworth::LowShelf<order> f;
        	f.setup(order, 44100.0, mCenterFrequency, mStrength/60.);
        	f.reset();
        	int32_t boostl = f.filter(dryL);
        	int32_t boostr = f.filter(dryR);
    write(out, i * 2, boostl);
    write(out, i * 2 + 1, boostr);
    	}
}

    return mEnable ? 0 : -ENODATA;
}
