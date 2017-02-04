//Do NOT modify this file!!!
#ifndef ANDROID_EFFECT_STEREOWIDE_H_
#define ANDROID_EFFECT_STEREOWIDE_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
/* 37cc2c00-dddd-11db-8577-0002a5d5c51c */
static const effect_uuid_t SL_IID_STEREOWIDE_ = { 0x37cc2c00, 0xdddd, 0x11db, 0x8577, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1c } };
const effect_uuid_t * const SL_IID_STEREOWIDE = &SL_IID_STEREOWIDE_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for stereowide effect */
typedef enum
{
    STEREOWIDE_PARAM_STRENGTH_SUPPORTED,
    STEREOWIDE_PARAM_STRENGTH,
    STEREOWIDE_PARAM_FINE_TUNE_FREQ
} t_stereowide_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_VIRTUALIZER_H_*/
