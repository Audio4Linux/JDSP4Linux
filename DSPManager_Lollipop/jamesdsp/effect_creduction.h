#ifndef ANDROID_EFFECT_REDUCTION_H_
#define ANDROID_EFFECT_REDUCTION_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_REDUCTION_ = { 0xd1c2bc8a, 0x56cd, 0x11e5, 0x885d,
		{ 0xfe, 0xff, 0x81, 0x9c, 0xdc, 0x9f } };
const effect_uuid_t * const SL_IID_REDUCTION = &SL_IID_REDUCTION_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for Reduction effect */
typedef enum
{
    REDUCTION_PARAM_STRENGTH_SUPPORTED,
    REDUCTION_PARAM_STRENGTH,
    REDUCTION_PARAM_HIGH_CENTER_FREQUENCY
} t_reduction_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_REDUCTION_H_*/
