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

#include "FIR16.h"

#include <string.h>

FIR16::FIR16()
    : mIndex(0)
{
    memset(mState, 0, sizeof(mState));
    memset(mCoeff, 0, sizeof(mCoeff));
}

FIR16::~FIR16()
{
}

void FIR16::setParameters(float coeff[16])
{
    for (int32_t i = 0; i < 16; i ++) {
        mCoeff[i] = int64_t(coeff[i] * (int64_t(1) << 32));
    }
}

int32_t FIR16::process(int32_t x0)
{
    mIndex --;
    mState[mIndex & 0xf] = x0;

    int64_t y = 0;
    for (int32_t i = 0; i < 16; i ++) {
        y += mCoeff[i] * mState[(i + mIndex) & 0xf];
    }

    return int32_t(y >> 32);
}
