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

#ifndef ANDROID_EFFECT_VIRTUALIZER_H_
#define ANDROID_EFFECT_VIRTUALIZER_H_

#include <hardware/audio_effect.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_VIRTUALIZER_ = { 0x37cc2c00, 0xdddd, 0x11db, 0x8577,
        { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const SL_IID_VIRTUALIZER = &SL_IID_VIRTUALIZER_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for virtualizer effect */
/* to keep in sync with frameworks/base/media/java/android/media/audiofx/Virtualizer.java */
typedef enum
{
    VIRTUALIZER_PARAM_STRENGTH_SUPPORTED,
    VIRTUALIZER_PARAM_STRENGTH,
    // used with EFFECT_CMD_GET_PARAM
    // format:
    //   parameters int32_t              VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES
    //              audio_channel_mask_t input channel mask
    //              audio_devices_t      audio output device
    //   output     int32_t*             an array of length 3 * the number of channels in the mask
    //                                       where entries are the succession of the channel mask
    //                                       of each speaker (i.e. a single bit is selected in the
    //                                       channel mask) followed by the azimuth and the
    //                                       elevation angles.
    //   status     int -EINVAL  if configuration is not supported or invalid or not forcing
    //                   0       if configuration is supported and the mode is forced
    // notes:
    // - all angles are expressed in degrees and are relative to the listener,
    // - for azimuth: 0 is the direction the listener faces, 180 is behind the listener, and
    //    -90 is to her/his left,
    // - for elevation: 0 is the horizontal plane, +90 is above the listener, -90 is below.
    VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES,
    // used with EFFECT_CMD_SET_PARAM
    // format:
    //   parameters  int32_t           VIRTUALIZER_PARAM_FORCE_VIRTUALIZATION_MODE
    //               audio_devices_t   audio output device
    //   status      int -EINVAL   if the device is not supported or invalid
    //                   0         if the device is supported and the mode is forced, or forcing
    //                               was disabled for the AUDIO_DEVICE_NONE audio device.
    VIRTUALIZER_PARAM_FORCE_VIRTUALIZATION_MODE,
    // used with EFFECT_CMD_GET_PARAM
    // format:
    //   parameters int32_t              VIRTUALIZER_PARAM_VIRTUALIZATION_MODE
    //   output     audio_device_t       audio device reflecting the current virtualization mode,
    //                                   AUDIO_DEVICE_NONE when not virtualizing
    //   status     int -EINVAL if an error occurred
    //                  0       if the output value is successfully retrieved
    VIRTUALIZER_PARAM_VIRTUALIZATION_MODE
} t_virtualizer_params;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_VIRTUALIZER_H_*/
