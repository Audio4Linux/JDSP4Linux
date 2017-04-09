//This is a working FFT filter
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
    : mStrength(0), mEchoDecay(1000.0f), mReverbMode(0), mReverbPreset(0)
{
    refreshStrength();
}
EffectVirtualizer::~EffectVirtualizer()
{
  if(left!=NULL) {
      LOGI("Free some shit!!!");
      NE10_FREE(left);
      NE10_FREE(right);
      NE10_FREE(cpxleft);
      NE10_FREE(cpxright);
      free(cfgFor);
      free(cfgInv);
  }
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
}

void EffectVirtualizer::printTest()
{
    LOGI("Reverb mode = %d",mReverbMode);
    LOGI("Reverb preset = %d",mReverbPreset);
}
int EffectVirtualizer::next_power_of_two(float a_F){
	    int f = *(int*)&a_F;
	    int b = f << 9 != 0;
	    f >>= 23;
	    f -= 127;
	    return (1 << (f + b));
}
int32_t EffectVirtualizer::process(audio_buffer_t* in, audio_buffer_t* out)
{
if(left==NULL)
{
    powTwoFrame=next_power_of_two(in->frameCount);
    LOGI("process() audio frame size = %u\nPower of two size = %d",in->frameCount, powTwoFrame);
    left = (ne10_int32_t*)NE10_MALLOC(powTwoFrame * sizeof(ne10_int32_t));
    right = (ne10_int32_t*)NE10_MALLOC(powTwoFrame * sizeof(ne10_int32_t));
    cpxleft = (ne10_fft_cpx_int32_t*)NE10_MALLOC(powTwoFrame * sizeof (ne10_fft_cpx_int32_t));
    cpxright = (ne10_fft_cpx_int32_t*)NE10_MALLOC(powTwoFrame * sizeof (ne10_fft_cpx_int32_t));
    cfgFor = ne10_fft_alloc_r2c_int32(powTwoFrame);
    cfgInv = ne10_fft_alloc_r2c_int32(powTwoFrame);
}
    for (uint32_t i = 0; i < in->frameCount; i ++) {
	left[i] = read(in, i * 2);
        right[i] = read(in, i * 2 + 1);
    }
    for (uint32_t i = in->frameCount; i < powTwoFrame; i ++) {
	left[i] = 0;
        right[i] = 0;
    }
    ne10_fft_r2c_1d_int32_neon(cpxleft, left, cfgFor, 1);
    ne10_fft_r2c_1d_int32_neon(cpxright, right, cfgFor, 1);
/*	for (int i=0;i<powTwoFrame/2;i++) //Direct frequency filtering test
	{
	    if(i>(mStrength/2.5)) {
		cpxleft[i].r = 0;
		cpxleft[i].i = 0;
		cpxright[i].r = 0;
		cpxright[i].i = 0;
	    }
	}*/
    ne10_fft_c2r_1d_int32_neon(left, cpxleft, cfgInv, 0);
    ne10_fft_c2r_1d_int32_neon(right, cpxright, cfgInv, 0);
    for (uint32_t i = 0; i < powTwoFrame; i ++) {
        write(out, i * 2, (int32_t)left[i]);
        write(out, i * 2 + 1, (int32_t)right[i]);
    }
    return mEnable ? 0 : -ENODATA;
}
