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

#include "Delay.h"

#include <string.h>

Delay::Delay()
    : mState(0), mIndex(0), mLength(0)
{
}

Delay::~Delay()
{
    if (mState != 0) {
        delete[] mState;
        mState = 0;
    }
}

void Delay::setParameters(float samplingFrequency, float time)
{
    mLength = int32_t(time * samplingFrequency + 0.5f);
    if (mState != 0) {
        delete[] mState;
    }
    mState = new int32_t[mLength];
    memset(mState, 0, mLength * sizeof(int32_t));
    mIndex = 0;
}

int32_t Delay::process(int32_t x0)
{
    int32_t y0 = mState[mIndex];
    mState[mIndex] = x0;
    mIndex = (mIndex + 1) % mLength;
    return y0;
}
