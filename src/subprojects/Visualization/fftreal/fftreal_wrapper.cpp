/***************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 2.1. This
** program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
** FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
** for more details. You should have received a copy of the GNU General
** Public License along with this program. If not, see
** <http://www.gnu.org/licenses/>.
**
***************************************************************************/

#include "fftreal_wrapper.h"

// FFTReal code generates quite a lot of 'unused parameter' compiler warnings,
// which we suppress here in order to get a clean build output.
#if defined Q_CC_MSVC
#    pragma warning(disable:4100)
#elif defined Q_CC_GNU
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined Q_CC_MWERKS
#    pragma warning off (10182)
#endif

#include "FFTRealFixLen.h"

class FFTRealWrapperPrivate {
public:
    ffft::FFTRealFixLen<FFTLengthPowerOfTwo> m_fft;
};


FFTRealWrapper::FFTRealWrapper()
    :   m_private(new FFTRealWrapperPrivate)
{

}

FFTRealWrapper::~FFTRealWrapper()
{
    delete m_private;
}

void FFTRealWrapper::calculateFFT(DataType in[], const DataType out[])
{
    m_private->m_fft.do_fft(in, out);
}
