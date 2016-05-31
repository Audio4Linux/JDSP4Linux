/*
 * Copyright (C) 2014 The Android Open Source Project
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
#ifndef AUDIO_POLICY_HELPER_H_
#define AUDIO_POLICY_HELPER_H_

#include <system/audio.h>

static audio_stream_type_t audio_attributes_to_stream_type(const audio_attributes_t *attr)
{
    // flags to stream type mapping
    if ((attr->flags & AUDIO_FLAG_AUDIBILITY_ENFORCED) == AUDIO_FLAG_AUDIBILITY_ENFORCED) {
        return AUDIO_STREAM_ENFORCED_AUDIBLE;
    }
    if ((attr->flags & AUDIO_FLAG_SCO) == AUDIO_FLAG_SCO) {
        return AUDIO_STREAM_BLUETOOTH_SCO;
    }

    // usage to stream type mapping
    switch (attr->usage) {
    case AUDIO_USAGE_MEDIA:
    case AUDIO_USAGE_GAME:
    case AUDIO_USAGE_ASSISTANCE_ACCESSIBILITY:
    case AUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE:
        return AUDIO_STREAM_MUSIC;
    case AUDIO_USAGE_ASSISTANCE_SONIFICATION:
        return AUDIO_STREAM_SYSTEM;
    case AUDIO_USAGE_VOICE_COMMUNICATION:
        return AUDIO_STREAM_VOICE_CALL;

    case AUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING:
        return AUDIO_STREAM_DTMF;

    case AUDIO_USAGE_ALARM:
        return AUDIO_STREAM_ALARM;
    case AUDIO_USAGE_NOTIFICATION_TELEPHONY_RINGTONE:
        return AUDIO_STREAM_RING;

    case AUDIO_USAGE_NOTIFICATION:
    case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_REQUEST:
    case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_INSTANT:
    case AUDIO_USAGE_NOTIFICATION_COMMUNICATION_DELAYED:
    case AUDIO_USAGE_NOTIFICATION_EVENT:
        return AUDIO_STREAM_NOTIFICATION;

    case AUDIO_USAGE_UNKNOWN:
    default:
        return AUDIO_STREAM_MUSIC;
    }
}

static void stream_type_to_audio_attributes(audio_stream_type_t streamType,
                                     audio_attributes_t *attr) {
    memset(attr, 0, sizeof(audio_attributes_t));

    switch (streamType) {
    case AUDIO_STREAM_DEFAULT:
    case AUDIO_STREAM_MUSIC:
        attr->content_type = AUDIO_CONTENT_TYPE_MUSIC;
        attr->usage = AUDIO_USAGE_MEDIA;
        break;
    case AUDIO_STREAM_VOICE_CALL:
        attr->content_type = AUDIO_CONTENT_TYPE_SPEECH;
        attr->usage = AUDIO_USAGE_VOICE_COMMUNICATION;
        break;
    case AUDIO_STREAM_ENFORCED_AUDIBLE:
        attr->flags  |= AUDIO_FLAG_AUDIBILITY_ENFORCED;
        // intended fall through, attributes in common with STREAM_SYSTEM
    case AUDIO_STREAM_SYSTEM:
        attr->content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
        attr->usage = AUDIO_USAGE_ASSISTANCE_SONIFICATION;
        break;
    case AUDIO_STREAM_RING:
        attr->content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
        attr->usage = AUDIO_USAGE_NOTIFICATION_TELEPHONY_RINGTONE;
        break;
    case AUDIO_STREAM_ALARM:
        attr->content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
        attr->usage = AUDIO_USAGE_ALARM;
        break;
    case AUDIO_STREAM_NOTIFICATION:
        attr->content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
        attr->usage = AUDIO_USAGE_NOTIFICATION;
        break;
    case AUDIO_STREAM_BLUETOOTH_SCO:
        attr->content_type = AUDIO_CONTENT_TYPE_SPEECH;
        attr->usage = AUDIO_USAGE_VOICE_COMMUNICATION;
        attr->flags |= AUDIO_FLAG_SCO;
        break;
    case AUDIO_STREAM_DTMF:
        attr->content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
        attr->usage = AUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING;
        break;
    case AUDIO_STREAM_TTS:
        attr->content_type = AUDIO_CONTENT_TYPE_SPEECH;
        attr->usage = AUDIO_USAGE_ASSISTANCE_ACCESSIBILITY;
        break;
    default:
        ALOGE("invalid stream type %d when converting to attributes", streamType);
    }
}

#endif //AUDIO_POLICY_HELPER_H_
