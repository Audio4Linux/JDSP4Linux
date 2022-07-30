/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_AUDIO_BASE_UTILS_H
#define ANDROID_AUDIO_BASE_UTILS_H

#include "audio-base.h"

/** Define helper values to iterate over enum, extend them or checking value validity.
 *  Those values are compatible with the O corresponding enum values.
 *  They are not macro like similar values in audio.h to avoid conflicting
 *  with the libhardware_legacy audio.h.
 */
enum {
    /** Number of audio stream available to vendors. */
    AUDIO_STREAM_PUBLIC_CNT = AUDIO_STREAM_ACCESSIBILITY + 1,

#ifndef AUDIO_NO_SYSTEM_DECLARATIONS
    /** Total number of stream handled by the policy*/
    AUDIO_STREAM_FOR_POLICY_CNT= AUDIO_STREAM_REROUTING + 1,
#endif

   /** Total number of stream. */
    AUDIO_STREAM_CNT          = AUDIO_STREAM_PATCH + 1,

    AUDIO_SOURCE_MAX          = AUDIO_SOURCE_UNPROCESSED,
    AUDIO_SOURCE_CNT          = AUDIO_SOURCE_MAX + 1,

    AUDIO_MODE_MAX            = AUDIO_MODE_IN_COMMUNICATION,
    AUDIO_MODE_CNT            = AUDIO_MODE_MAX + 1,

    /** For retrocompatibility AUDIO_MODE_* and AUDIO_STREAM_* must be signed. */
    AUDIO_DETAIL_NEGATIVE_VALUE = -1,
};

enum {
    AUDIO_CHANNEL_OUT_ALL     = AUDIO_CHANNEL_OUT_FRONT_LEFT |
                                AUDIO_CHANNEL_OUT_FRONT_RIGHT |
                                AUDIO_CHANNEL_OUT_FRONT_CENTER |
                                AUDIO_CHANNEL_OUT_LOW_FREQUENCY |
                                AUDIO_CHANNEL_OUT_BACK_LEFT |
                                AUDIO_CHANNEL_OUT_BACK_RIGHT |
                                AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER |
                                AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER |
                                AUDIO_CHANNEL_OUT_BACK_CENTER |
                                AUDIO_CHANNEL_OUT_SIDE_LEFT |
                                AUDIO_CHANNEL_OUT_SIDE_RIGHT |
                                AUDIO_CHANNEL_OUT_TOP_CENTER |
                                AUDIO_CHANNEL_OUT_TOP_FRONT_LEFT |
                                AUDIO_CHANNEL_OUT_TOP_FRONT_CENTER |
                                AUDIO_CHANNEL_OUT_TOP_FRONT_RIGHT |
                                AUDIO_CHANNEL_OUT_TOP_BACK_LEFT |
                                AUDIO_CHANNEL_OUT_TOP_BACK_CENTER |
                                AUDIO_CHANNEL_OUT_TOP_BACK_RIGHT |
                                AUDIO_CHANNEL_OUT_TOP_SIDE_LEFT |
                                AUDIO_CHANNEL_OUT_TOP_SIDE_RIGHT,

    AUDIO_CHANNEL_IN_ALL      = AUDIO_CHANNEL_IN_LEFT |
                                AUDIO_CHANNEL_IN_RIGHT |
                                AUDIO_CHANNEL_IN_FRONT |
                                AUDIO_CHANNEL_IN_BACK|
                                AUDIO_CHANNEL_IN_LEFT_PROCESSED |
                                AUDIO_CHANNEL_IN_RIGHT_PROCESSED |
                                AUDIO_CHANNEL_IN_FRONT_PROCESSED |
                                AUDIO_CHANNEL_IN_BACK_PROCESSED|
                                AUDIO_CHANNEL_IN_PRESSURE |
                                AUDIO_CHANNEL_IN_X_AXIS |
                                AUDIO_CHANNEL_IN_Y_AXIS |
                                AUDIO_CHANNEL_IN_Z_AXIS |
                                AUDIO_CHANNEL_IN_VOICE_UPLINK |
                                AUDIO_CHANNEL_IN_VOICE_DNLINK |
                                AUDIO_CHANNEL_IN_BACK_LEFT |
                                AUDIO_CHANNEL_IN_BACK_RIGHT |
                                AUDIO_CHANNEL_IN_CENTER |
                                AUDIO_CHANNEL_IN_LOW_FREQUENCY |
                                AUDIO_CHANNEL_IN_TOP_LEFT |
                                AUDIO_CHANNEL_IN_TOP_RIGHT,

    AUDIO_DEVICE_OUT_ALL      = AUDIO_DEVICE_OUT_EARPIECE |
                                AUDIO_DEVICE_OUT_SPEAKER |
                                AUDIO_DEVICE_OUT_WIRED_HEADSET |
                                AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                                AUDIO_DEVICE_OUT_BLUETOOTH_SCO |
                                AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                                AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT |
                                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |
                                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |
                                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER |
                                AUDIO_DEVICE_OUT_HDMI |
                                AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
                                AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
                                AUDIO_DEVICE_OUT_USB_ACCESSORY |
                                AUDIO_DEVICE_OUT_USB_DEVICE |
                                AUDIO_DEVICE_OUT_REMOTE_SUBMIX |
                                AUDIO_DEVICE_OUT_TELEPHONY_TX |
                                AUDIO_DEVICE_OUT_LINE |
                                AUDIO_DEVICE_OUT_HDMI_ARC |
                                AUDIO_DEVICE_OUT_SPDIF |
                                AUDIO_DEVICE_OUT_FM |
                                AUDIO_DEVICE_OUT_AUX_LINE |
                                AUDIO_DEVICE_OUT_SPEAKER_SAFE |
                                AUDIO_DEVICE_OUT_IP |
                                AUDIO_DEVICE_OUT_BUS |
                                AUDIO_DEVICE_OUT_PROXY |
                                AUDIO_DEVICE_OUT_USB_HEADSET |
                                AUDIO_DEVICE_OUT_HEARING_AID |
                                AUDIO_DEVICE_OUT_ECHO_CANCELLER |
                                AUDIO_DEVICE_OUT_DEFAULT,

    AUDIO_DEVICE_OUT_ALL_A2DP = AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |
                                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |
                                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER,

    AUDIO_DEVICE_OUT_ALL_SCO  = AUDIO_DEVICE_OUT_BLUETOOTH_SCO |
                                AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                                AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT,

    AUDIO_DEVICE_OUT_ALL_USB  = AUDIO_DEVICE_OUT_USB_ACCESSORY |
                                AUDIO_DEVICE_OUT_USB_DEVICE |
                                AUDIO_DEVICE_OUT_USB_HEADSET,

    AUDIO_DEVICE_IN_ALL       = AUDIO_DEVICE_IN_COMMUNICATION |
                                AUDIO_DEVICE_IN_AMBIENT |
                                AUDIO_DEVICE_IN_BUILTIN_MIC |
                                AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET |
                                AUDIO_DEVICE_IN_WIRED_HEADSET |
                                AUDIO_DEVICE_IN_HDMI |
                                AUDIO_DEVICE_IN_TELEPHONY_RX |
                                AUDIO_DEVICE_IN_BACK_MIC |
                                AUDIO_DEVICE_IN_REMOTE_SUBMIX |
                                AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET |
                                AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET |
                                AUDIO_DEVICE_IN_USB_ACCESSORY |
                                AUDIO_DEVICE_IN_USB_DEVICE |
                                AUDIO_DEVICE_IN_FM_TUNER |
                                AUDIO_DEVICE_IN_TV_TUNER |
                                AUDIO_DEVICE_IN_LINE |
                                AUDIO_DEVICE_IN_SPDIF |
                                AUDIO_DEVICE_IN_BLUETOOTH_A2DP |
                                AUDIO_DEVICE_IN_LOOPBACK |
                                AUDIO_DEVICE_IN_IP |
                                AUDIO_DEVICE_IN_BUS |
                                AUDIO_DEVICE_IN_PROXY |
                                AUDIO_DEVICE_IN_USB_HEADSET |
                                AUDIO_DEVICE_IN_BLUETOOTH_BLE |
                                AUDIO_DEVICE_IN_DEFAULT,

    AUDIO_DEVICE_IN_ALL_SCO   = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET,

    AUDIO_DEVICE_IN_ALL_USB   = AUDIO_DEVICE_IN_USB_ACCESSORY |
                                AUDIO_DEVICE_IN_USB_DEVICE |
                                AUDIO_DEVICE_IN_USB_HEADSET,

    AUDIO_USAGE_MAX           = AUDIO_USAGE_ASSISTANT,
    AUDIO_USAGE_CNT           = AUDIO_USAGE_ASSISTANT + 1,

    AUDIO_PORT_CONFIG_ALL     = AUDIO_PORT_CONFIG_SAMPLE_RATE |
                                AUDIO_PORT_CONFIG_CHANNEL_MASK |
                                AUDIO_PORT_CONFIG_FORMAT |
                                AUDIO_PORT_CONFIG_GAIN,
}; // enum


#endif  // ANDROID_AUDIO_BASE_UTILS_H
