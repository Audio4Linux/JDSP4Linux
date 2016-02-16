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

#ifndef ANDROID_EFFECT_AEC_H_
#define ANDROID_EFFECT_AEC_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

// The AEC type UUID is not defined by OpenSL ES and has been generated from
// http://www.itu.int/ITU-T/asn1/uuid.html
static const effect_uuid_t FX_IID_AEC_ =
    { 0x7b491460, 0x8d4d, 0x11e0, 0xbd61, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const FX_IID_AEC = &FX_IID_AEC_;

typedef enum
{
    AEC_PARAM_ECHO_DELAY,           // echo delay in microseconds
    AEC_PARAM_PROPERTIES
} t_aec_params;

//t_equalizer_settings groups all current aec settings for backup and restore.
typedef struct s_aec_settings {
    uint32_t echoDelay;
} t_aec_settings;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_AEC_H_*/
