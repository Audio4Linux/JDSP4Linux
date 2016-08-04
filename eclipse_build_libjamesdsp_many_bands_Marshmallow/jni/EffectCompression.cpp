#include "EffectCompression.h"

#include <math.h>

typedef struct {
        effect_param_t ep;
        uint32_t code;
        uint16_t value;
} cmd1x4_1x2_t;

static int32_t max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

EffectCompression::EffectCompression()
    : mCompressionRatio(2.0), mFade(0)
{
    for (int32_t i = 0; i < 2; i ++) {
        mCurrentLevel[i] = 0;
        mUserLevel[i] = 1 << 24;
    }
}

int32_t EffectCompression::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t *replyData = (int32_t *) pReplyData;
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            *replyData = ret;
            return 0;
        }

        /* This filter gives a reasonable approximation of A- and C-weighting
         * which is close to correct for 100 - 10 kHz. 10 dB gain must be added to result. */
        mWeigherBP[0].setBandPass(0, 2200, mSamplingRate, 0.33);
        mWeigherBP[1].setBandPass(0, 2200, mSamplingRate, 0.33);

        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_SET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t *replyData = (int32_t *) pReplyData;
            cmd1x4_1x2_t *strength = (cmd1x4_1x2_t *) pCmdData;
            if (strength->code == 0) {
                /* 1.0 .. 11.0 */
                mCompressionRatio = 1.f + strength->value / 100.f;
                *replyData = 0;
                return 0;
            }
        }
        return -1;
    }

    if (cmdCode == EFFECT_CMD_SET_VOLUME && cmdSize == 8) {
        if (pReplyData != NULL) {
            int32_t *userVols = (int32_t *) pCmdData;
            for (uint32_t i = 0; i < cmdSize / 4; i ++) {
                mUserLevel[i] = userVols[i];
            }

            int32_t *myVols = (int32_t *) pReplyData;
            for (uint32_t i = 0; i < *replySize / 4; i ++) {
                myVols[i] = 1 << 24; /* Unity gain */
            }
        } else {
            /* We don't control volume. */
            for (int32_t i = 0; i < 2; i ++) {
                mUserLevel[i] = 1 << 24;
            }
        }

        return 0;
    }

    /* Init to current volume level on enabling effect to prevent
     * initial fade in / other shite */
    if (cmdCode == EFFECT_CMD_ENABLE) {
        /* Unfortunately Android calls SET_VOLUME after ENABLE for us.
         * so we can't really use those volumes. It's safest just to fade in
         * each time. */
        for (int32_t i = 0; i < 2; i ++) {
             mCurrentLevel[i] = 0;
        }
    }

    return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}

/* Return fixed point 16.48 */
uint64_t EffectCompression::estimateOneChannelLevel(audio_buffer_t *in, int32_t interleave, int32_t offset, Biquad4proc& weigherBP)
{
    uint64_t power = 0;
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        int32_t tmp = read(in, offset);
        tmp = weigherBP.process(tmp);

        /* 2^24 * 2^24 = 48 */
        power += int64_t(tmp) * int64_t(tmp);
        offset += interleave;
    }

    return (power / in->frameCount);
}

int32_t EffectCompression::process(audio_buffer_t *in, audio_buffer_t *out)
{
    /* Analyze both channels separately, pick the maximum power measured. */
    uint64_t maximumPowerSquared = 0;
    for (uint32_t i = 0; i < 2; i ++) {
        uint64_t candidatePowerSquared = estimateOneChannelLevel(in, 2, i, mWeigherBP[i]);
        if (candidatePowerSquared > maximumPowerSquared) {
            maximumPowerSquared = candidatePowerSquared;
        }
    }

    /* -100 .. 0 dB. */
    float signalPowerDb = logf(maximumPowerSquared / float(int64_t(1) << 48) + 1e-10f) / logf(10.0f) * 10.0f;

    /* Target 83 dB SPL */
    signalPowerDb += 96.0f - 83.0f + 10.0f;

    /* now we have an estimate of the signal power, with 0 level around 83 dB.
     * we now select the level to boost to. */
    float desiredLevelDb = signalPowerDb / mCompressionRatio;

    /* turn back to multiplier */
    float correctionDb = desiredLevelDb - signalPowerDb;

    if (mEnable && mFade != 100) {
        mFade += 1;
    }
    if (!mEnable && mFade != 0) {
        mFade -= 1;
    }

    correctionDb *= mFade / 100.f;

    /* Reduce extreme boost by a smooth ramp.
     * New range -50 .. 0 dB */
    correctionDb -= powf(correctionDb/100, 2.0f) * (100.0f / 2.0f);

    /* 40.24 */
    int64_t correctionFactor = (1 << 24) * powf(10.0f, correctionDb / 20.0f);

    /* Now we have correction factor and user-desired sound level. */
    for (uint32_t i = 0; i < 2; i ++) {
        /* 8.24 */
        int32_t desiredLevel = mUserLevel[i] * correctionFactor >> 24;

        /* 8.24 */
        int32_t volAdj = desiredLevel - mCurrentLevel[i];

        /* I want volume adjustments to occur in about 0.025 seconds.
         * However, if the input buffer would happen to be longer than
         * this, I'll just make sure that I am done with the adjustment
         * by the end of it. */
        int32_t adjLen = mSamplingRate / 40; // in practice, about 1100 frames
        /* This formulation results in piecewise linear approximation of
         * exponential because the rate of adjustment decreases from granule
         * to granule. */
        volAdj /= max(adjLen, in->frameCount);

        /* Additionally, I want volume to increase only very slowly.
         * This biases us against pumping effects and also tends to spare
         * our ears when some very loud sound begins suddenly. */
        if (volAdj > 0) {
            volAdj >>= 4;
        }

        for (uint32_t j = 0; j < in->frameCount; j ++) {
             int32_t value = read(in, j * 2 + i);
             value = int64_t(value) * mCurrentLevel[i] >> 24;
             write(out, j * 2 + i, value);
             mCurrentLevel[i] += volAdj;
        }
    }

    return mEnable || mFade != 0 ? 0 : -ENODATA;
}
