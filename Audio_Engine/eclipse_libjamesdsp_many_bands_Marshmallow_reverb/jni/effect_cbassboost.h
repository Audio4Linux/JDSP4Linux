//Do NOT modify this file!!!
#ifndef ANDROID_EFFECT_BASSBOOST_H_
#define ANDROID_EFFECT_BASSBOOST_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_BASSBOOST_ = { 0x42b5cbf5, 0x4dd8, 0x4e79, 0xa5fb,
		{ 0xcc, 0xeb, 0x2c, 0xb5, 0x4e, 0x13 } };
const effect_uuid_t * const SL_IID_BASSBOOST = &SL_IID_BASSBOOST_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for BassBoost effect */
typedef enum
{
    BASSBOOST_PARAM_STRENGTH_SUPPORTED,
    BASSBOOST_PARAM_STRENGTH,
    BASSBOOST_PARAM_FILTER_SLOPE,
    BASSBOOST_PARAM_CENTER_FREQUENCY
} t_bassboost_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_BASSBOOST_H_*/
