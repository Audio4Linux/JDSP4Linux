/*
 * Copyright (C) 2011 The Android Open Source Project
 * Modifications Copyright (C) 2013 The OmniROM Project
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

#ifndef ANDROID_EFFECT_STEREOWIDE_H_
#define ANDROID_EFFECT_STEREOWIDE_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
/* 37cc2c00-dddd-11db-8577-0002a5d5c51c */
static const effect_uuid_t SL_IID_STEREOWIDE_ = { 0x37cc2c00, 0xdddd, 0x11db, 0x8577,
        { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1c } };
const effect_uuid_t * const SL_IID_STEREOWIDE = &SL_IID_STEREOWIDE_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for stereowide effect */
typedef enum
{
    STEREOWIDE_PARAM_STRENGTH_SUPPORTED,
    STEREOWIDE_PARAM_STRENGTH
} t_stereowide_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_VIRTUALIZER_H_*/
