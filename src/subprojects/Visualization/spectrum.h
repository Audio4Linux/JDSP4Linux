/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "fftreal_wrapper.h" // For FFTLengthPowerOfTwo
#include "utils.h"
#include <qglobal.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

// Number of audio samples used to calculate the frequency spectrum
const int   SpectrumLengthSamples      = PowerOfTwo<FFTLengthPowerOfTwo>::Result;

// Number of bands in the frequency spectrum
const int   SpectrumNumBands           = 70;

// Lower bound of first band in the spectrum
const qreal SpectrumLowFreq            = 0.0; // Hz

// Upper band of last band in the spectrum
const qreal SpectrumHighFreq           = 1000.0; // Hz

// Fudge factor used to calculate the spectrum bar heights
const qreal SpectrumAnalyserMultiplier = 0.15;


// -----------------------------------------------------------------------------
// Types and data structures
// -----------------------------------------------------------------------------

enum WindowFunction
{
	NoWindow,
	HannWindow
};

const WindowFunction DefaultWindowFunction = HannWindow;

struct Tone
{
	Tone(qreal freq = 0.0,
	     qreal amp  = 0.0)
		:   frequency(freq), amplitude(amp)
	{}

	// Start and end frequencies for swept tone generation
	qreal frequency;

	// Amplitude in range [0.0, 1.0]
	qreal amplitude;
};

struct SweptTone
{
	SweptTone(qreal start = 0.0,
	          qreal end   = 0.0,
	          qreal amp   = 0.0)
		:   startFreq(start), endFreq(end), amplitude(amp)
	{ Q_ASSERT(end >= start); }

	SweptTone(const Tone &tone)
		:   startFreq(tone.frequency), endFreq(tone.frequency), amplitude(tone.amplitude)
	{}

	// Start and end frequencies for swept tone generation
	qreal startFreq;
	qreal endFreq;

	// Amplitude in range [0.0, 1.0]
	qreal amplitude;
};


// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

// Macro which connects a signal to a slot, and which causes application to
// abort if the connection fails.  This is intended to catch programming errors
// such as mis-typing a signal or slot name.  It is necessary to write our own
// macro to do this - the following idiom
//     Q_ASSERT(connect(source, signal, receiver, slot));
// will not work because Q_ASSERT compiles to a no-op in release builds.

#define CHECKED_CONNECT(source, signal, receiver, slot) \
	if (!connect(source, signal, receiver, slot))       \
	qt_assert_x (Q_FUNC_INFO, "CHECKED_CONNECT failed", __FILE__, __LINE__) ;

// Handle some dependencies between macros defined in the .pro file

#ifdef DISABLE_WAVEFORM
#undef SUPERIMPOSE_PROGRESS_ON_WAVEFORM
#endif

#endif // SPECTRUM_H