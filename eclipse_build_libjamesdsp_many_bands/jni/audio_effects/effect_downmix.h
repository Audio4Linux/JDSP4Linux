/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_EFFECT_DOWNMIX_H_
#define ANDROID_EFFECT_DOWNMIX_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#define EFFECT_UIID_DOWNMIX__ { 0x381e49cc, 0xa858, 0x4aa2, 0x87f6, \
                              { 0xe8, 0x38, 0x8e, 0x76, 0x01, 0xb2 } }
static const effect_uuid_t EFFECT_UIID_DOWNMIX_ = EFFECT_UIID_DOWNMIX__;
const effect_uuid_t * const EFFECT_UIID_DOWNMIX = &EFFECT_UIID_DOWNMIX_;


/* enumerated parameter settings for downmix effect */
typedef enum {
    DOWNMIX_PARAM_TYPE
} downmix_params_t;


typedef enum {
    DOWNMIX_TYPE_INVALID                 = -1,
    // throw away the extra channels
    DOWNMIX_TYPE_STRIP                   = 0,
    // mix the extra channels with FL/FR
    DOWNMIX_TYPE_FOLD                    = 1,
    DOWNMIX_TYPE_CNT,
    DOWNMIX_TYPE_LAST = DOWNMIX_TYPE_CNT - 1
} downmix_type_t;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_DOWNMIX_H_*/
