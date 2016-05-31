/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_EFFECT_AGC_H_
#define ANDROID_EFFECT_AGC_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

// The AGC type UUID is not defined by OpenSL ES and has been generated from
// http://www.itu.int/ITU-T/asn1/uuid.html
static const effect_uuid_t FX_IID_AGC_ =
    { 0x0a8abfe0, 0x654c, 0x11e0, 0xba26, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const FX_IID_AGC = &FX_IID_AGC_;


typedef enum
{
    AGC_PARAM_TARGET_LEVEL,      // target output level in millibel
    AGC_PARAM_COMP_GAIN,         // gain in the compression range in millibel
    AGC_PARAM_LIMITER_ENA,       // enable or disable limiter (boolean)
    AGC_PARAM_PROPERTIES
} t_agc_params;


//t_agc_settings groups all current agc settings for backup and restore.
typedef struct s_agc_settings {
    int16_t  targetLevel;
    int16_t  compGain;
    bool     limiterEnabled;
} t_agc_settings;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_AGC_H_*/
