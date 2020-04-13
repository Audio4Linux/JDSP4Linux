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
 *  Modified version of biquad.h from https://github.com/ThePBone/DDCToolbox
 */

#ifndef BIQUAD_H
#define BIQUAD_H
#include <complex>
#include <cmath>
#include <cfloat>
#include <cstdint>

class biquad
{
public:
    enum Type
    {
        PEAKING = 0x00,
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS1,
        BAND_PASS2,
        NOTCH,
        ALL_PASS,
        LOW_SHELF,
        HIGH_SHELF,
        UNITY_GAIN,
        ONEPOLE_LOWPASS,
        ONEPOLE_HIGHPASS,
        INVALID = 0xFF
    };
    biquad();
    void refreshFilter(uint32_t id, Type type,double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS, bool isBandwidthOrS);
    std::complex<double> evaluateTransfer(std::complex<double> z);
    uint32_t getId();

private:
    std::complex<double> internalBiquadCoeffs[6];
    double m_dFilterBQ;
    double m_dFilterFreq;
    double m_dFilterGain;
    Type m_dFilterType;
    bool m_isBandwidthOrS;
    uint32_t m_id;
};

#endif // BIQUAD_H
