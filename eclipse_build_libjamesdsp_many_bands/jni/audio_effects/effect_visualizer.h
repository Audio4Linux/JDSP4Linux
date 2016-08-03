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

#ifndef ANDROID_EFFECT_VISUALIZER_H_
#define ANDROID_EFFECT_VISUALIZER_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_VISUALIZATION_ =
    { 0xe46b26a0, 0xdddd, 0x11db, 0x8afd, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const SL_IID_VISUALIZATION = &SL_IID_VISUALIZATION_;
#endif //OPENSL_ES_H_

#define VISUALIZER_CAPTURE_SIZE_MAX 1024  // maximum capture size in samples
#define VISUALIZER_CAPTURE_SIZE_MIN 128   // minimum capture size in samples

// to keep in sync with frameworks/base/media/java/android/media/audiofx/Visualizer.java
#define VISUALIZER_SCALING_MODE_NORMALIZED 0
#define VISUALIZER_SCALING_MODE_AS_PLAYED  1

#define MEASUREMENT_MODE_NONE      0x0
#define MEASUREMENT_MODE_PEAK_RMS  0x1

#define MEASUREMENT_IDX_PEAK 0
#define MEASUREMENT_IDX_RMS  1

/* enumerated parameters for Visualizer effect */
typedef enum
{
    VISUALIZER_PARAM_CAPTURE_SIZE, // Sets the number PCM samples in the capture.
    VISUALIZER_PARAM_SCALING_MODE, // Sets the way the captured data is scaled
    VISUALIZER_PARAM_LATENCY,      // Informs the visualizer about the downstream latency
    VISUALIZER_PARAM_MEASUREMENT_MODE, // Sets which measurements are to be made
} t_visualizer_params;

/* commands */
typedef enum
{
    VISUALIZER_CMD_CAPTURE = EFFECT_CMD_FIRST_PROPRIETARY, // Gets the latest PCM capture.
    VISUALIZER_CMD_MEASURE, // Gets the current measurements
}t_visualizer_cmds;

// VISUALIZER_CMD_CAPTURE retrieves the latest PCM snapshot captured by the visualizer engine.
// It returns the number of samples specified by VISUALIZER_PARAM_CAPTURE_SIZE
// in 8 bit unsigned format (0 = 0x80)

// VISUALIZER_CMD_MEASURE retrieves the lastest measurements as int32_t saved in the
// MEASUREMENT_IDX_* array index order.

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_VISUALIZER_H_*/
