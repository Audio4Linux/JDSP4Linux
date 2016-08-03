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

#ifndef ANDROID_EFFECT_NS_H_
#define ANDROID_EFFECT_NS_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

// The NS type UUID is not defined by OpenSL ES and has been generated from
// http://www.itu.int/ITU-T/asn1/uuid.html
static const effect_uuid_t FX_IID_NS_ =
    { 0x58b4b260, 0x8e06, 0x11e0, 0xaa8e, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const FX_IID_NS = &FX_IID_NS_;

typedef enum
{
    NS_PARAM_LEVEL,             // noise suppression level (t_ns_level)
    NS_PARAM_PROPERTIES
} t_ns_params;

// noise suppression level
typedef enum {
    NS_LEVEL_LOW,
    NS_LEVEL_MEDIUM,
    NS_LEVEL_HIGH
} t_ns_level;

// s_ns_settings groups all current ns settings for backup and restore.
typedef struct s_ns_settings {
    uint32_t  level;
} t_ns_settings;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_NS_H_*/
