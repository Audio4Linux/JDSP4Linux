/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 *  Modified version of biquad.cpp from https://github.com/ThePBone/DDCToolbox
 */

#include "biquad.h"
#include <cstdio>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

biquad::biquad()
{
    internalBiquadCoeffs[0] = std::complex<double>(0,0);
    internalBiquadCoeffs[1] = std::complex<double>(0,0);
    internalBiquadCoeffs[2] = std::complex<double>(0,0);
    internalBiquadCoeffs[3] = std::complex<double>(0,0);
    internalBiquadCoeffs[4] = std::complex<double>(0,0);
    internalBiquadCoeffs[5] = std::complex<double>(0,0);
}

uint32_t biquad::getId(){
    return m_id;
}

void biquad::refreshFilter(uint32_t id, Type type, double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS, bool isBandwidthOrS)
{
    m_dFilterType = type;
    m_dFilterGain = dbGain;
    m_dFilterFreq = centreFreq;
    m_dFilterBQ = dBandwidthOrQOrS;
    m_isBandwidthOrS = isBandwidthOrS;
    m_id = id;

    double d;
    if (type == PEAKING || type == LOW_SHELF || type == HIGH_SHELF)
        d = pow(10.0, dbGain / 40.0);
    else
        d = pow(10.0, dbGain / 20.0);
    double a = (6.2831853071795862 * centreFreq) / fs; //omega
    double num3 = sin(a); //sn
    double cs = cos(a);

    double alpha;
    if (!isBandwidthOrS) // Q
        alpha = num3 / (2 * dBandwidthOrQOrS);
    else if (type == LOW_SHELF || type == HIGH_SHELF) // S
        alpha = num3 / 2 * sqrt((d + 1 / d) * (1 / dBandwidthOrQOrS - 1) + 2);
    else // BW
        alpha = num3 * sinh(0.693147180559945309417 / 2 * dBandwidthOrQOrS * a / num3);

    double beta = 2 * sqrt(d) * alpha;
    double B0 = 0.0, B1 = 0.0, B2 = 0.0, A0 = 0.0, A1 = 0.0, A2 = 0.0;

    switch (type)
    {
    case LOW_PASS:
        B0 = (1.0 - cs) / 2.0;
        B1 = 1.0 - cs;
        B2 = (1.0 - cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case HIGH_PASS:
        B0 = (1.0 + cs) / 2.0;
        B1 = -(1.0 + cs);
        B2 = (1.0 + cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case BAND_PASS1:
        //BPF, constant skirt gain (peak gain = BW)
        B0 = dBandwidthOrQOrS * alpha;// sn / 2;
        B1 = 0;
        B2 = -dBandwidthOrQOrS * alpha;//-sn / 2;
        A0 = 1 + alpha;
        A1 = -2 * cs;
        A2 = 1 - alpha;
        break;
    case BAND_PASS2:
        //BPF, constant 0dB peak gain
        B0 = alpha;
        B1 = 0.0;
        B2 = -alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case NOTCH:
        B0 = 1.0;
        B1 = -2.0 * cs;
        B2 = 1.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case ALL_PASS:
        B0 = 1.0 - alpha;
        B1 = -2.0 * cs;
        B2 = 1.0 + alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case PEAKING:
        B0 = 1.0 + (alpha * d);
        B1 = -2.0 * cs;
        B2 = 1.0 - (alpha * d);
        A0 = 1.0 + (alpha / d);
        A1 = -2.0 * cs;
        A2 = 1.0 - (alpha / d);
        break;
    case LOW_SHELF:
        B0 = d * ((d + 1.0) - (d - 1.0) * cs + beta);
        B1 = 2.0 * d * ((d - 1.0) - (d + 1.0) * cs);
        B2 = d * ((d + 1.0) - (d - 1.0) * cs - beta);
        A0 = (d + 1.0) + (d - 1.0) * cs + beta;
        A1 = -2.0 * ((d - 1.0) + (d + 1.0) * cs);
        A2 = (d + 1.0) + (d - 1.0) * cs - beta;
        break;
    case HIGH_SHELF:
        B0 = d * ((d + 1.0) + (d - 1.0) * cs + beta);
        B1 = -2.0 * d * ((d - 1.0) + (d + 1.0) * cs);
        B2 = d * ((d + 1.0) + (d - 1.0) * cs - beta);
        A0 = (d + 1.0) - (d - 1.0) * cs + beta;
        A1 = 2.0 * ((d - 1.0) - (d + 1.0) * cs);
        A2 = (d + 1.0) - (d - 1.0) * cs - beta;
        break;
    case UNITY_GAIN:
        B0 = d;
        B1 = 0.0;
        B2 = 0.0;
        A0 = 1.0;
        A1 = 0.0;
        A2 = 0.0;
        break;
    case ONEPOLE_LOWPASS:
        B0 = tan(M_PI * centreFreq / fs * 0.5);
        A1 = -(1.0 - B0) / (1.0 + B0);
        B1 = B0 = B0 / (1.0 + B0);
        B2 = 0.0;
        A0 = 1.0;
        A2 = 0.0;
        break;
    case ONEPOLE_HIGHPASS:
        B0 = tan(M_PI * centreFreq / fs * 0.5);
        A1 = -(1.0 - B0) / (1.0 + B0);
        B0 = 1.0 - (B0 / (1.0 + B0));
        B2 = 0.0;
        B1 = -B0;
        A0 = 1.0;
        A2 = 0.0;
        break;
    case INVALID:
        break;
    }

    internalBiquadCoeffs[0] = B0;
    internalBiquadCoeffs[1] = B1;
    internalBiquadCoeffs[2] = B2;
    internalBiquadCoeffs[3] = A0;
    internalBiquadCoeffs[4] = A1;
    internalBiquadCoeffs[5] = A2;
}

complex<double> biquad::evaluateTransfer(complex<double> z) {
     auto zSquared = z*z;
     auto nom = internalBiquadCoeffs[0]+(internalBiquadCoeffs[1]/z)+(internalBiquadCoeffs[2]/zSquared);
     auto den = internalBiquadCoeffs[3]+(internalBiquadCoeffs[4]/z)+(internalBiquadCoeffs[5]/zSquared);
     return nom / den;
 }
