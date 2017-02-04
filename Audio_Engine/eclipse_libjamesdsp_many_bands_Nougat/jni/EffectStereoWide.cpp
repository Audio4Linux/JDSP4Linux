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
 * We assume we have an MS matrix, and we basically shift towards
 * S rather than M, cutting down the center channel, and enhancing the
 * separate L/R channels (slightly).
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
        break;

    case 1: // Slight
        mMatrixMCoeff = 0.95;
        mMatrixSCoeff = 1.4;
        break;

    case 2: // Moderate
        mMatrixMCoeff = 0.90;
        mMatrixSCoeff = 1.7;
        break;

    case 3: // High
        mMatrixMCoeff = 0.80;
        mMatrixSCoeff = 2.0;
        break;

    case 4: // Super
        mMatrixMCoeff = 0.70;
        mMatrixSCoeff = 2.5;
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
        M = M * mMatrixMCoeff;
        S = S * mMatrixSCoeff;

        /* Final mix */
        write(out, i * 2, M+S);
        write(out, i * 2 + 1, M-S);
    }

    return mEnable ? 0 : -ENODATA;
}

