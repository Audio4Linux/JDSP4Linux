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


#ifndef ANDROID_AUDIO_POLICY_H
#define ANDROID_AUDIO_POLICY_H

#include "../hardware/system/audio.h"
#include "../hardware/system/audio_policy.h"
#include "binder/Parcel.h"
#include "binder/utils/String8.h"
#include "binder/utils/Vector.h"

namespace android
{

// Keep in sync with AudioMix.java, AudioMixingRule.java, AudioPolicyConfig.java
#define RULE_EXCLUSION_MASK 0x8000
#define RULE_MATCH_ATTRIBUTE_USAGE 0x1
#define RULE_MATCH_ATTRIBUTE_CAPTURE_PRESET (0x1 << 1)
#define RULE_EXCLUDE_ATTRIBUTE_USAGE (RULE_EXCLUSION_MASK|RULE_MATCH_ATTRIBUTE_USAGE)
#define RULE_EXCLUDE_ATTRIBUTE_CAPTURE_PRESET \
    (RULE_EXCLUSION_MASK|RULE_MATCH_ATTRIBUTE_CAPTURE_PRESET)

#define MIX_TYPE_INVALID -1
#define MIX_TYPE_PLAYERS 0
#define MIX_TYPE_RECORDERS 1

#define ROUTE_FLAG_RENDER 0x1
#define ROUTE_FLAG_LOOP_BACK (0x1 << 1)

#define MAX_MIXES_PER_POLICY 10
#define MAX_CRITERIA_PER_MIX 20

class AttributeMatchCriterion
{
public:
    AttributeMatchCriterion() {}
    AttributeMatchCriterion(audio_usage_t usage, audio_source_t source, uint32_t rule);

    status_t readFromParcel(Parcel *parcel);
    status_t writeToParcel(Parcel *parcel) const;

    union
    {
        audio_usage_t   mUsage;
        audio_source_t  mSource;
    } mAttr;
    uint32_t        mRule;
};

class AudioMix
{
public:
    AudioMix() {}
    AudioMix(Vector<AttributeMatchCriterion> criteria, uint32_t mixType, audio_config_t format,
             uint32_t routeFlags, String8 registrationId) :
        mCriteria(criteria), mMixType(mixType), mFormat(format),
        mRouteFlags(routeFlags), mRegistrationId(registrationId) {}

    status_t readFromParcel(Parcel *parcel);
    status_t writeToParcel(Parcel *parcel) const;

    Vector<AttributeMatchCriterion> mCriteria;
    uint32_t        mMixType;
    audio_config_t  mFormat;
    uint32_t        mRouteFlags;
    String8         mRegistrationId;
};

}; // namespace android

#endif  // ANDROID_AUDIO_POLICY_H
