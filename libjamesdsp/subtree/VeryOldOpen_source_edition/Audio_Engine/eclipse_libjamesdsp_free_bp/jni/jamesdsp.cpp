#include <string.h>
#include <media/AudioEffect.h>
#include <hardware/audio_effect.h>

#include "Effect.h"
#include "EffectDSPMain.h"

static effect_descriptor_t jamesdsp_descriptor =
{
	{ 0xf98765f4, 0xc321, 0x5de6, 0x9a45, { 0x12, 0x34, 0x59, 0x49, 0x5a, 0xb2 } },
    { 0xf27317f4, 0xc984, 0x4de6, 0x9a90, { 0x54, 0x57, 0x59, 0x49, 0x5b, 0xf2 } }, // own UUID
    EFFECT_CONTROL_API_VERSION,
	EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_FIRST,
	10,
    1,
    "JamesDSP main unit",
    "James34602"
};

/* Library mandatory methods. */
extern "C" {
    struct effect_module_s
    {
        const struct effect_interface_s *itfe;
        Effect *effect;
        effect_descriptor_t *descriptor;
    };
    static int32_t generic_process(effect_handle_t self, audio_buffer_t *in, audio_buffer_t *out)
    {
        struct effect_module_s *e = (struct effect_module_s *) self;
        return e->effect->process(in, out);
    }
    static int32_t generic_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData)
    {
        struct effect_module_s *e = (struct effect_module_s *) self;
        return e->effect->command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
    }
    static int32_t generic_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
    {
        struct effect_module_s *e = (struct effect_module_s *) self;
        memcpy(pDescriptor, e->descriptor, sizeof(effect_descriptor_t));
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
        struct effect_module_s *e = (struct effect_module_s *) calloc(1, sizeof(struct effect_module_s));
        e->itfe = &generic_interface;
        e->effect = new EffectDSPMain();
        e->descriptor = &jamesdsp_descriptor;
        *pEffect = (effect_handle_t) e;
        return 0;
    }
    int32_t EffectRelease(effect_handle_t ei)
    {
        struct effect_module_s *e = (struct effect_module_s *) ei;
        delete e->effect;
        free(e);
        return 0;
    }
    int32_t EffectGetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
    {
    	size_t descriptorSize = sizeof(effect_descriptor_t);
        memcpy(pDescriptor, &jamesdsp_descriptor, descriptorSize);
        return 0;
    }
    audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM =
    {
        .tag = AUDIO_EFFECT_LIBRARY_TAG,
        .version = EFFECT_LIBRARY_API_VERSION,
        .name = "James34602 Effect Library",
        .implementor = "James34602",
        .create_effect = EffectCreate,
        .release_effect = EffectRelease,
        .get_descriptor = EffectGetDescriptor,
    };
}
