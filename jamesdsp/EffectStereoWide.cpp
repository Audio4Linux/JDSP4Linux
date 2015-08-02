#include <math.h>

#include "EffectStereoWide.h"

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

/**
 * Explanation of the effect:
 *
 * In order to achieve a stereo widening effect, we use three methods
 * combined in one effect that can be toggled on the user interface.
 *
 * First, we assume we have an MS matrix, and we basically shift towards
 * S rather than M, cutting down the center channel, and enhancing the
 * separate L/R channels (slightly).
 *
 * Second, for songs that might not have a good stereo image, we
 * apply a short delay on the L or R channel (after high-pass)
 * to simlate a stereo effect. This is the most noticeable effect.
 *
 * Finally, the third method is based on a split EQ effect. We basically
 * apply an EQ only to the S part of the MS matrix, making the S signal
 * sound crisper, thus giving a subtle boost to the stereo image.
 *
 */

EffectStereoWide::EffectStereoWide()
    : mStrength(0)
{
    refreshStrength();
}

int32_t EffectStereoWide::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            int32_t *replyData = (int32_t *) pReplyData;
            *replyData = ret;
            return 0;
        }

        /* We set a high pass at 2kHz to cut off bass. Because bass stays in the
         * center channel. The Q is set to 1.0 to keep a smooth transition.
         */
        mHighPass.setHighPass(0, 2000.0f, mSamplingRate, 1.0f);

        /* Bass trim, see usage in process
         */
        mBassTrim.setLowPass(0, 70.0f, mSamplingRate, 1.0f);

        mDelayData = 0;

        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_GET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == STEREOWIDE_PARAM_STRENGTH_SUPPORTED) {
                reply1x4_1x4_t *replyData = (reply1x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = 1;
                *replySize = sizeof(reply1x4_1x4_t);
                return 0;
            }
            if (cmd == STEREOWIDE_PARAM_STRENGTH) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mStrength;
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
            if (cmd == STEREOWIDE_PARAM_STRENGTH) {
                mStrength = ((int16_t *) cep)[8];
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

void EffectStereoWide::refreshStrength()
{
    switch (mStrength) {
    case 0: // A Bit
        mMatrixMCoeff = 1.0;
        mMatrixSCoeff = 1.2;
        mSplitEQCoeff = 0.1;
        mSplitEQCompCoeff = 0.02;
        mBassTrimCoeff = 0.1;
        break;

    case 1: // Slight
        mMatrixMCoeff = 0.95;
        mMatrixSCoeff = 1.4;
        mSplitEQCoeff = 0.2;
        mSplitEQCompCoeff = 0.05;
        mBassTrimCoeff = 0.12;
        break;

    case 2: // Moderate
        mMatrixMCoeff = 0.90;
        mMatrixSCoeff = 1.6;
        mSplitEQCoeff = 0.3;
        mSplitEQCompCoeff = 0.15;
        mBassTrimCoeff = 0.13;
        break;

    case 3: // High
        mMatrixMCoeff = 0.80;
        mMatrixSCoeff = 1.8;
        mSplitEQCoeff = 0.4;
        mSplitEQCompCoeff = 0.21;
        mBassTrimCoeff = 0.18;
        break;

    case 4: // Super
        mMatrixMCoeff = 0.70;
        mMatrixSCoeff = 2.0;
        mSplitEQCoeff = 0.6;
        mSplitEQCompCoeff = 0.32;
        mBassTrimCoeff = 0.21;
        break;
    }
}

int32_t EffectStereoWide::process(audio_buffer_t* in, audio_buffer_t* out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
        int32_t dataL = dryL;
        int32_t dataR = dryR;

        /* To simulate our MS matrix, we extract the center channel and
         * the sides in separate variables
         */
        /* Center channel. */
        int32_t M  = (dataL + dataR) >> 1;
        /* Direct radiation components. */
        int32_t S = (dataL - dataR) >> 1;

        /* First pass: We turn down M and boost S. Note that we
         * don't go through the High Pass here, as we use the
         * original stereo image
         */
        M = M * mMatrixMCoeff;
        S = S * mMatrixSCoeff;

        /* Calculate the high pass now. Note that we have the
         * high pass in a separate variable that we can add to
         * our existing signals, so that acts similarly to an EQ.
         * We could add another BandPass EQ to act just on one
         * specific range of frequencies, but that's not needed here.
         * We're on Android, not in a recording studio.
         */
        int32_t highPass = mHighPass.process(S);

        /* And here's our split EQ */
        S += highPass * mSplitEQCoeff;

        /* We compensate this pass in the center channel to avoid clipping
         * and to avoid acting like an EQ.
         */
        M -= highPass * mSplitEQCompCoeff;

        /* Is it worth noting that enhancing the S channel of the MS
         * matrix slightly reduces the bass impact. We trim bass a
         * little bit with a small low pass on the M channel (don't ever
         * enhance bass on S!)
         */
        int32_t lowPass = mBassTrim.process(M);
        M += lowPass * mBassTrimCoeff;

        /* Last, to enhance noticeably the stereo image, we delay
         * both channels */
        // XXX: It sounds good this way already. Maybe it can be implemented later,
        // but as of now, there's no need IMHO.

        /* Final mix */
        write(out, i * 2, M+S);
        write(out, i * 2 + 1, M-S);
    }

    return mEnable ? 0 : -ENODATA;
}

