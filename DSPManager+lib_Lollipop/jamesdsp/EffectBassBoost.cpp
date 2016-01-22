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
    : mStrength(0), mFilterType(0), mAlgorithm(0), mCenterFrequency(55.0f)
{
    refreshStrength();
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
            if (cmd == BASSBOOST_PARAM_FILTER_TYPE) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mFilterType;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_ALGORITHM) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mAlgorithm;
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
            if (cmd == BASSBOOST_PARAM_FILTER_TYPE) {
                mFilterType = ((int16_t* )cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == BASSBOOST_PARAM_ALGORITHM) {
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
    /* Q = 0.5 .. 2.0 */
    mBoost.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
    mBoostL.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
    mBoostR.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
}
else if(mFilterType == 1)
{
    mBoost.setLowPassPeak(0, mCenterFrequency, mSamplingRate, 1.5f + mStrength / 580.0f);
    mBoostL.setLowPassPeak(0, mCenterFrequency, mSamplingRate, 1.5f + mStrength / 580.0f);
    mBoostR.setLowPassPeak(0, mCenterFrequency, mSamplingRate, 1.5f + mStrength / 580.0f);
}
else 
{
   mBoost.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
   mBoostL.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
   mBoostR.setLowPass(0, mCenterFrequency, mSamplingRate, 0.5f + mStrength / 666.0f);
}
}

int32_t EffectBassBoost::process(audio_buffer_t* in, audio_buffer_t* out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
        /* Original LVM effect was far more involved than this one.
         * This effect is mostly a placeholder until I port that, or
         * something else. LVM process diagram was as follows:
         *
         * in -> [ HPF ] -+-> [ mono mix ] -> [ BPF ] -> [ compressor ] -> out
         *                `-->------------------------------>--'
         *
         * High-pass filter was optional, and seemed to be
         * tuned at 55 Hz and upwards. BPF is probably always tuned
         * at the same frequency, as this would make sense.
         *
         * Additionally, a compressor element was used to limit the
         * mixing of the boost (only!) to avoid clipping.
	*/
if(mAlgorithm == 0)
{
	if (mStrength >= 600)
{
    noiseon = triangularDither8() / 2;
}
else
{
    noiseon = 0;
}
	int32_t boostl = mBoostL.process(dryL);
	int32_t boostr = mBoostR.process(dryR);
        write(out, i * 2, dryL + boostl + noiseon);
        write(out, i * 2 + 1, dryR + boostr + noiseon);
}
else if(mAlgorithm == 1)
{
	if (mStrength >= 600)
{
    noiseon = triangularDither8() / 2;
}
else
{
    noiseon = 0;
}
	int32_t boost = mBoost.process(dryL + dryR);
        write(out, i * 2, dryL + boost + noiseon);
        write(out, i * 2 + 1, dryR + boost + noiseon);
}
else 
{
	if (mStrength >= 600)
{
    noiseon = triangularDither8() / 2;
}
else
{
    noiseon = 0;
}
	int32_t boostl = mBoostL.process(dryL);
	int32_t boostr = mBoostR.process(dryR);
        write(out, i * 2, dryL + boostl + noiseon);
        write(out, i * 2 + 1, dryR + boostr + noiseon);
}
}

    return mEnable ? 0 : -ENODATA;
}

