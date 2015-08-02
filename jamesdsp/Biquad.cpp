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
 *
 * Per article 5 of the Apache 2.0 License, some modifications to this code
 * were made by the OmniROM Project.
 *
 * Modifications Copyright (C) 2013 The OmniROM Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "Biquad.h"
#include <math.h>

static int64_t toFixedPoint(double in) {
    return int64_t(0.5 + in * (int64_t(1) << 32));
}

Biquad::Biquad()
{
    reset();
    setCoefficients(0, 1, 0, 0, 1, 0, 0);
}

Biquad::~Biquad()
{
}

void Biquad::setCoefficients(int32_t steps, double a0, double a1, double a2, double b0, double b1, double b2)
{
    int64_t A1 = -toFixedPoint(a1/a0);
    int64_t A2 = -toFixedPoint(a2/a0);
    int64_t B0 = toFixedPoint(b0/a0);
    int64_t B1 = toFixedPoint(b1/a0);
    int64_t B2 = toFixedPoint(b2/a0);

    if (steps == 0) {
        mA1 = A1;
        mA2 = A2;
        mB0 = B0;
        mB1 = B1;
        mB2 = B2;
        mInterpolationSteps = 0;
    } else {
        mA1dif = (A1 - mA1) / steps;
        mA2dif = (A2 - mA2) / steps;
        mB0dif = (B0 - mB0) / steps;
        mB1dif = (B1 - mB1) / steps;
        mB2dif = (B2 - mB2) / steps;
        mInterpolationSteps = steps;
    }
}

void Biquad::reset()
{
    mInterpolationSteps = 0;
    mA1 = 0;
    mA2 = 0;
    mB0 = 0;
    mB1 = 0;
    mB2 = 0;
    mX1 = 0;
    mX2 = 0;
    mY1 = 0;
    mY2 = 0;
}

void Biquad::setHighShelf(int32_t steps, double center_frequency, double sampling_frequency, double gainDb, double slope, double overallGainDb)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double A = pow(10, gainDb/40);
    double alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/slope - 1) + 2 );

    double b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha );
    double b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   );
    double b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha );
    double a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha  ;
    double a1 =    2*( (A-1) - (A+1)*cos(w0)                   );
    double a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha  ;

    double overallGain = pow(10, overallGainDb / 20);
    b0 *= overallGain;
    b1 *= overallGain;
    b2 *= overallGain;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setBandPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =   sin(w0)/2;
    double b1 =   0;
    double b2 =  -sin(w0)/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setHighPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =  (1 + cos(w0))/2;
    double b1 = -(1 + cos(w0));
    double b2 =  (1 + cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setLowPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =  (1 - cos(w0))/2;
    double b1 =   1 - cos(w0);
    double b2 =  (1 - cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

int32_t Biquad::process(int32_t x0)
{
    int64_t y0 = mB0 * x0
        + mB1 * mX1
        + mB2 * mX2
        + mA1 * mY1
        + mA2 * mY2;
    y0 >>= 32;

    mY2 = mY1;
    mY1 = y0;

    mX2 = mX1;
    mX1 = x0;

    /* Interpolate biquad parameters */
    if (mInterpolationSteps != 0) {
        mInterpolationSteps --;
        mB0 += mB0dif;
        mB1 += mB1dif;
        mB2 += mB2dif;
        mA1 += mA1dif;
        mA2 += mA2dif;
    }

    return y0;
}
