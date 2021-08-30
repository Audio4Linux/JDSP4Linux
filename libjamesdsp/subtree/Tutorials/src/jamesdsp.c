#define TAG "jamesdsp:"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "essential.h"

const effect_descriptor_t jamesdsp_descriptor =
{
	{ 0xf98765f4, 0xc321, 0x5de6, 0x9a45, { 0x12, 0x34, 0x59, 0x49, 0x5a, 0xb2 } },
	{ 0xf27317f4, 0xc984, 0x4de6, 0x9a90, { 0x54, 0x57, 0x59, 0x49, 0x5b, 0xf2 } }, // own UUID
	EFFECT_CONTROL_API_VERSION,
	EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_FIRST,
	10,
	1,
	"DSP Main",
	"James34602"
};
__attribute__((constructor)) static void initialize(void)
{
	LOGI("Initialization: DLL loaded");
}
__attribute__((destructor)) static void destruction(void)
{
	LOGI("Initialization: Unload DLL");
}
typedef struct
{
	int mEnable;
} Effect;
/* Library mandatory methods. */
struct effect_module_s
{
	const struct effect_interface_s *itfe;
	Effect jdsp;
};
int counter;
static int32_t generic_process(effect_handle_t self, audio_buffer_t *in, audio_buffer_t *out)
{
	struct effect_module_s *e = (struct effect_module_s *) self;
	Effect *jdsp = &e->jdsp;
	int i, actualFrameCount = in->frameCount;
	counter++;
	if (counter == 128)
	{
		LOGI("Processing loop: Processing %d samples", actualFrameCount);
		counter = 0;
	}
	return jdsp->mEnable ? 0 : -ENODATA;
}
int32_t formatConfigure(void* pCmdData, effect_buffer_access_e* mAccessMode)
{
	int mSamplingRate, formatFloatModeInt32Mode;
	effect_config_t *cfg = (effect_config_t*)pCmdData;
	buffer_config_t in = cfg->inputCfg;
	buffer_config_t out = cfg->outputCfg;
	/* Check that we aren't asked to do resampling. Note that audioflinger
	 * always provides full setup info at initial configure time. */
	LOGE("Sample rate of In: %u and out: %u", in.samplingRate, out.samplingRate);
	if ((in.mask & EFFECT_CONFIG_SMP_RATE) && (out.mask & EFFECT_CONFIG_SMP_RATE))
	{
		if (out.samplingRate != in.samplingRate)
		{
			LOGE("In/out sample rate doesn't match");
			return -EINVAL;
		}
		mSamplingRate = in.samplingRate;
	}
	if (in.mask & EFFECT_CONFIG_CHANNELS && out.mask & EFFECT_CONFIG_CHANNELS)
	{
		if (in.channels != AUDIO_CHANNEL_OUT_STEREO)
		{
			LOGE("Input is non stereo signal. It's channel count is %u", in.channels);
			return -EINVAL;
		}
		if (out.channels != AUDIO_CHANNEL_OUT_STEREO)
		{
			LOGE("Output is non stereo signal. It's channel count is %u", out.channels);
			return -EINVAL;
		}
	}
	else
	{
		LOGE("In/out channel mask doesn't match");
	}
	if (in.mask & EFFECT_CONFIG_FORMAT)
	{
		if (in.format != AUDIO_FORMAT_PCM_16_BIT)
		{
			if (in.format == AUDIO_FORMAT_PCM_FLOAT)
				formatFloatModeInt32Mode = 1;
			else if (in.format == AUDIO_FORMAT_PCM_32_BIT)
				formatFloatModeInt32Mode = 2;
			LOGE("Input is not 16 bit PCM. FMT is %u", in.format);
		}
	}
	if (out.mask & EFFECT_CONFIG_FORMAT)
	{
		if (out.format != AUDIO_FORMAT_PCM_16_BIT)
		{
			if (out.format == AUDIO_FORMAT_PCM_FLOAT)
				formatFloatModeInt32Mode = 1;
			else if (in.format == AUDIO_FORMAT_PCM_32_BIT)
				formatFloatModeInt32Mode = 2;
			LOGE("Output is not 16 bit PCM. FMT is %u", in.format);
		}
	}
	if (out.mask & EFFECT_CONFIG_ACC_MODE)
		*mAccessMode = (effect_buffer_access_e)out.accessMode;
	return 0;
}
static int32_t generic_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData)
{
	struct effect_module_s *e = (struct effect_module_s *) self;
	Effect *jdsp = &e->jdsp;
	if (cmdCode == EFFECT_CMD_SET_CONFIG)
	{
		effect_buffer_access_e mAccessMode;
		int32_t *replyData = (int32_t *)pReplyData;
		int32_t ret = formatConfigure(pCmdData, &mAccessMode);
		if (ret != 0)
		{
			*replyData = ret;
			return 0;
		}
		*replyData = 0;
		return 0;
	}
	if (cmdCode == EFFECT_CMD_GET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
	}
	switch (cmdCode)
	{
	case EFFECT_CMD_ENABLE:
	case EFFECT_CMD_DISABLE:
	{
		jdsp->mEnable = cmdCode == EFFECT_CMD_ENABLE;
		int32_t *replyData = (int32_t *)pReplyData;
		*replyData = 0;
		break;
	}
	case EFFECT_CMD_INIT:
	case EFFECT_CMD_SET_CONFIG:
	case EFFECT_CMD_SET_PARAM:
	case EFFECT_CMD_SET_PARAM_COMMIT:
	{
		int32_t *replyData = (int32_t *)pReplyData;
		*replyData = 0;
		break;
	}
	case EFFECT_CMD_RESET:
	case EFFECT_CMD_SET_PARAM_DEFERRED:
	case EFFECT_CMD_SET_DEVICE:
	case EFFECT_CMD_SET_AUDIO_MODE:
		break;
	case EFFECT_CMD_GET_PARAM:
	{
		effect_param_t *rep = (effect_param_t *)pReplyData;
		rep->status = -EINVAL;
		rep->psize = 0;
		rep->vsize = 0;
		*replySize = 12;
		break;
	}
	}
	return 0;
}
static int32_t generic_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
	LOGI("Effect control interface: Copying descriptor info");
	if (pDescriptor == NULL)
	{
		LOGI("Effect control interface: generic_getDescriptor() called with NULL pointer");
		return -EINVAL;
	}
	*pDescriptor = jamesdsp_descriptor;
	return 0;
}
static const struct effect_interface_s generic_interface =
{
	generic_process,
	generic_command,
	generic_getDescriptor,
	NULL
};
int32_t EffectCreate(const effect_uuid_t *uuid, int32_t sessionId, int32_t ioId, effect_handle_t *pEffect)
{
	LOGI("Initialization: Creating effect with %d sessionId", sessionId);
	// Debug init start
	counter = 0;
	// Debug init end
	struct effect_module_s *e = (struct effect_module_s *) calloc(1, sizeof(struct effect_module_s));
	e->itfe = &generic_interface;
	// Initialize effect here
	*pEffect = (effect_handle_t)e;
	LOGI("Initialization: Effect created");
	return 0;
}
int32_t EffectRelease(effect_handle_t ei)
{
	struct effect_module_s *e = (struct effect_module_s *) ei;
	free(e);
	return 0;
}
int32_t EffectGetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
	LOGI("Initialization: Copying descriptor info");
	if (pDescriptor == NULL || uuid == NULL)
	{
		LOGI("Initialization: EffectGetDescriptor() called with NULL pointer");
		return -EINVAL;
	}
	*pDescriptor = jamesdsp_descriptor;
	return 0;
}
__attribute__((visibility("default"))) audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM =
{
	.tag = AUDIO_EFFECT_LIBRARY_TAG,
	.version = EFFECT_LIBRARY_API_VERSION,
#if __aarch64__ == 1
		.name = "James Audio DSP arm64",
#elif __ARM_ARCH_7A__ == 1
		.name = "James Audio DSP arm32",
#elif __i386__ == 1
		.name = "James Audio DSP x86",
#elif __x86_64__ == 1
		.name = "James Audio DSP x64",
#endif
		.implementor = "James34602",
		.create_effect = EffectCreate,
		.release_effect = EffectRelease,
		.get_descriptor = EffectGetDescriptor,
};
