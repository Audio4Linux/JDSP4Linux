#ifndef ANDROID_EFFECT_AMPLIFIER_H_
#define ANDROID_EFFECT_AMPLIFIER_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_AMPLIFIER_ = { 0x98c8baf0, 0x23a4, 0x4ce8, 0x8699,
		{ 0x6b, 0x18, 0x53, 0x96, 0x9e, 0x9f } };
const effect_uuid_t * const SL_IID_AMPLIFIER = &SL_IID_AMPLIFIER_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for AMPLIFIER effect */
typedef enum
{
    AMPLIFIER_PARAM_STRENGTH_SUPPORTED,
    AMPLIFIER_PARAM_STRENGTH
} t_AMPLIFIER_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_AMPLIFIER_H_*/
