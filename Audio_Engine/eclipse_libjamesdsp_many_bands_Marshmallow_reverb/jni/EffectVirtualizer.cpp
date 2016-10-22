#define TAG "Virtualizer"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#include <math.h>

#include "EffectVirtualizer.h"

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

EffectVirtualizer::EffectVirtualizer()
    : mStrength(0), mEchoDecay(1000.0f), mReverbMode(0),mReverbPreset(0)
{
    refreshStrength();
}

int32_t EffectVirtualizer::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            int32_t *replyData = (int32_t *) pReplyData;
            *replyData = ret;
            return 0;
        }

        /* Haas effect delay -- slight difference between L & R
         * to reduce artificialness of the ping-pong. */
        mReverbDelayL.setParameters(mSamplingRate, 0.030f);
        mReverbDelayR.setParameters(mSamplingRate, 0.024f);
        /* the -3 dB point is around 650 Hz, giving about 300 us to work with */
        mLocalization.setHighShelf(0, 800.0f, mSamplingRate, -11.5f, 0.72f);

        mDelayDataL = 0;
        mDelayDataR = 0;

        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_GET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == VIRTUALIZER_PARAM_STRENGTH_SUPPORTED) {
                reply1x4_1x4_t *replyData = (reply1x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = 1;
                *replySize = sizeof(reply1x4_1x4_t);
                return 0;
            }
            if (cmd == VIRTUALIZER_PARAM_STRENGTH) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mStrength;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
	    if (cmd == VIRTUALIZER_PARAM_ECHO_DECAY) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mEchoDecay;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
	    if (cmd == VIRTUALIZER_PARAM_REVERB_MODE) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mReverbMode;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
	    if (cmd == VIRTUALIZER_PARAM_RE_PRESET) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = (int16_t) mReverbPreset;
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
            if (cmd == VIRTUALIZER_PARAM_STRENGTH) {
                mStrength = ((int16_t *) cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == VIRTUALIZER_PARAM_ECHO_DECAY) {
                mEchoDecay = ((int16_t *) cep)[8];
                refreshStrength();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == VIRTUALIZER_PARAM_REVERB_MODE) {
                mReverbMode = ((int16_t *) cep)[8];
                printTest();
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
            if (cmd == VIRTUALIZER_PARAM_RE_PRESET) {
                mReverbPreset = ((int16_t *) cep)[8];
                printTest();
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

void EffectVirtualizer::refreshStrength()
{
    mDeep = mStrength != 0;
    mWide = mStrength >= 500;

    if (mStrength != 0) {
        float start = -15.0f;
        float end = -5.0f;
        float attenuation = start + (end - start) * (mStrength / mEchoDecay);
        float roomEcho = powf(10.0f, attenuation / 20.0f);
        mLevel = int64_t(roomEcho * (int64_t(1) << 32));
    } else {
        mLevel = 0;
    }
}

void EffectVirtualizer::printTest()
{
    LOGI("Reverb mode = %d",mReverbMode);
    LOGI("Reverb preset = %d",mReverbPreset);
}

int32_t EffectVirtualizer::process(audio_buffer_t* in, audio_buffer_t* out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        /* calculate reverb wet into dataL, dataR */
        int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
        int32_t dataL = dryL;
        int32_t dataR = dryR;

        if (mDeep) {
            /* Note: a pinking filter here would be good. */
            dataL += mDelayDataR;
            dataR += mDelayDataL;
        }

        dataL = mReverbDelayL.process(dataL);
        dataR = mReverbDelayR.process(dataR);

        if (mWide) {
            dataR = -dataR;
        }

        dataL = dataL * mLevel >> 32;
        dataR = dataR * mLevel >> 32;

        mDelayDataL = dataL;
        mDelayDataR = dataR;

        /* Reverb wet done; mix with dry and do headphone virtualization */
        dataL += dryL;
        dataR += dryR;

        /* Center channel. */
        int32_t center  = (dataL + dataR) >> 1;
        /* Direct radiation components. */
        int32_t side = (dataL - dataR) >> 1;

        /* Adjust derived center channel coloration to emphasize forward
         * direction impression. (XXX: disabled until configurable). */
        //center = mColorization.process(center);
        /* Sound reaching ear from the opposite speaker */
        side -= mLocalization.process(side);

        write(out, i * 2, center + side);
        write(out, i * 2 + 1, center - side);
    }

    return mEnable ? 0 : -ENODATA;
}

