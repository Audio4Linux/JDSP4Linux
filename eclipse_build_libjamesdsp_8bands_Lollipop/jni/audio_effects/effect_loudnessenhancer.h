/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ANDROID_EFFECT_LOUDNESS_ENHANCER_H_
#define ANDROID_EFFECT_LOUDNESS_ENHANCER_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

// this effect is not defined in OpenSL ES as one of the standard effects
static const effect_uuid_t FX_IID_LOUDNESS_ENHANCER_ =
        {0xfe3199be, 0xaed0, 0x413f, 0x87bb, {0x11, 0x26, 0x0e, 0xb6, 0x3c, 0xf1}};
const effect_uuid_t * const FX_IID_LOUDNESS_ENHANCER = &FX_IID_LOUDNESS_ENHANCER_;

#define LOUDNESS_ENHANCER_DEFAULT_TARGET_GAIN_MB 0 // mB

// enumerated parameters for DRC effect
// to keep in sync with frameworks/base/media/java/android/media/audiofx/LoudnessEnhancer.java
typedef enum
{
    LOUDNESS_ENHANCER_PARAM_TARGET_GAIN_MB = 0,// target gain expressed in mB
} t_level_monitor_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_LOUDNESS_ENHANCER_H_*/
