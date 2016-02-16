#ifndef ANDROID_EFFECT_BASSBOOST_H_
#define ANDROID_EFFECT_BASSBOOST_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_BASSBOOST_ = { 0x0634f220, 0xddd4, 0x11db, 0xa0fc,
        { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const SL_IID_BASSBOOST = &SL_IID_BASSBOOST_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for BassBoost effect */
typedef enum
{
    BASSBOOST_PARAM_STRENGTH_SUPPORTED,
    BASSBOOST_PARAM_STRENGTH,
    BASSBOOST_PARAM_FILTER_TYPE,
    BASSBOOST_PARAM_CENTER_FREQUENCY
} t_bassboost_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_BASSBOOST_H_*/
