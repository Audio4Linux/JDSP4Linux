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

#include "fftreal_wrapper.h"
#include "spectrumanalyser.h"
#include "utils.h"

#include <qmath.h>
#include <qmetatype.h>
#include <QAudioFormat>
#include <QThread>

SpectrumAnalyserThread::SpectrumAnalyserThread(QObject *parent)
	:   QObject(parent)
#ifndef DISABLE_FFT
	,   m_fft(new FFTRealWrapper)
#endif
	,   m_numSamples(SpectrumLengthSamples)
	,   m_windowFunction(DefaultWindowFunction)
	,   m_window(SpectrumLengthSamples, 0.0)
	,   m_input(SpectrumLengthSamples, 0.0)
	,   m_output(SpectrumLengthSamples, 0.0)
	,   m_spectrum(SpectrumLengthSamples)
#ifdef SPECTRUM_ANALYSER_SEPARATE_THREAD
	,   m_thread(new QThread(this))
#endif
{
#ifdef SPECTRUM_ANALYSER_SEPARATE_THREAD
	// moveToThread() cannot be called on a QObject with a parent
	setParent(0);
	moveToThread(m_thread);
	m_thread->start();
#endif
	calculateWindow();
}

SpectrumAnalyserThread::~SpectrumAnalyserThread()
{
#ifndef DISABLE_FFT
	delete m_fft;
#endif
}

void SpectrumAnalyserThread::setWindowFunction(WindowFunction type)
{
	m_windowFunction = type;
	calculateWindow();
}

void SpectrumAnalyserThread::calculateWindow()
{
	for (int i = 0; i < m_numSamples; ++i)
	{
		DataType x = 0.0;

		switch (m_windowFunction)
		{
			case NoWindow:
				x = 1.0;
				break;
			case HannWindow:
				x = 0.5 * (1 - qCos((2 * M_PI * i) / (m_numSamples - 1)));
				break;
			default:
				Q_ASSERT(false);
		}

		m_window[i] = x;
	}
}

void SpectrumAnalyserThread::calculateSpectrum(const QByteArray &buffer,
                                               int               inputFrequency,
                                               int               bytesPerSample,
                                               qreal             multiplier)
{
#ifndef DISABLE_FFT
	Q_ASSERT(buffer.size() == m_numSamples * bytesPerSample);

	// Initialize data array
	const char *ptr = buffer.constData();

	for (int i = 0; i < m_numSamples; ++i)
	{
		const qint16   pcmSample      = *reinterpret_cast<const qint16*>(ptr);
		// Scale down to range [-1.0, 1.0]
		const DataType realSample     = pcmToReal(pcmSample);
		const DataType windowedSample = realSample * m_window[i];
		m_input[i] = windowedSample;
		ptr       += bytesPerSample;
	}

	// Calculate the FFT
	m_fft->calculateFFT(m_output.data(), m_input.data());

	// Analyze output to obtain amplitude and phase for each frequency
	for (int i = 2; i <= m_numSamples / 2; ++i)
	{
		// Calculate frequency of this complex sample
		m_spectrum[i].frequency = qreal(i * inputFrequency) / (m_numSamples);

		const qreal real = m_output[i];
		qreal       imag = 0.0;

		if (i > 0 && i < m_numSamples / 2)
		{
			imag = m_output[m_numSamples / 2 + i];
		}

		const qreal magnitude = qSqrt(real * real + imag * imag);
		qreal       amplitude = multiplier * qLn(magnitude);

		// Bound amplitude to [0.0, 1.0]
		m_spectrum[i].clipped   = (amplitude > 1.0);
		amplitude               = qMax(qreal(0.0), amplitude);
		amplitude               = qMin(qreal(1.0), amplitude);
		m_spectrum[i].amplitude = amplitude;
	}

#endif

	emit calculationComplete(m_spectrum);
}

// =============================================================================
// SpectrumAnalyser
// =============================================================================

SpectrumAnalyser::SpectrumAnalyser(QObject *parent)
	:   QObject(parent)
	,   m_thread(new SpectrumAnalyserThread(this))
	,   m_state(Idle)
#ifdef DUMP_SPECTRUMANALYSER
	,   m_count(0)
#endif
{
	CHECKED_CONNECT(m_thread, SIGNAL(calculationComplete(FrequencySpectrum)),
	                this, SLOT(calculationComplete(FrequencySpectrum)));
}

SpectrumAnalyser::~SpectrumAnalyser()
{}

#ifdef DUMP_SPECTRUMANALYSER
void SpectrumAnalyser::setOutputPath(const QString &outputDir)
{
	m_outputDir.setPath(outputDir);
	m_textFile.setFileName(m_outputDir.filePath("spectrum.txt"));
	m_textFile.open(QIODevice::WriteOnly | QIODevice::Text);
	m_textStream.setDevice(&m_textFile);
}

#endif

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

void SpectrumAnalyser::setWindowFunction(WindowFunction type)
{
	const bool b = QMetaObject::invokeMethod(m_thread, "setWindowFunction",
	                                         Qt::AutoConnection,
	                                         Q_ARG(WindowFunction, type));
	Q_ASSERT(b);
	Q_UNUSED(b) // suppress warnings in release builds
}

void SpectrumAnalyser::calculate(const QByteArray &  buffer,
                                 const QAudioFormat &format,
                                 qreal               multiplier)
{
	// QThread::currentThread is marked 'for internal use only', but
	// we're only using it for debug output here, so it's probably OK :)
	SPECTRUMANALYSER_DEBUG << "SpectrumAnalyser::calculate"
	                       << QThread::currentThread()
	                       << "state" << m_state;

	if (isReady())
	{
		Q_ASSERT(isPCMS16LE(format));

		const int     bytesPerSample = format.sampleSize() * format.channelCount() / 8;

#ifdef DUMP_SPECTRUMANALYSER
		m_count++;
		const QString pcmFileName  = m_outputDir.filePath(QString("spectrum_%1.pcm").arg(m_count, 4, 10, QChar('0')));
		QFile         pcmFile(pcmFileName);
		pcmFile.open(QIODevice::WriteOnly);
		const int     bufferLength = m_numSamples * bytesPerSample;
		pcmFile.write(buffer, bufferLength);

		m_textStream << "TimeDomain " << m_count << "\n";
		const qint16 *input = reinterpret_cast<const qint16*>(buffer);

		for (int i = 0; i < m_numSamples; ++i)
		{
			m_textStream << i << "\t" << *input << "\n";
			input += format.channels();
		}

#endif

		m_state = Busy;

		// Invoke SpectrumAnalyserThread::calculateSpectrum using QMetaObject.  If
		// m_thread is in a different thread from the current thread, the
		// calculation will be done in the child thread.
		// Once the calculation is finished, a calculationChanged signal will be
		// emitted by m_thread.

		const bool b = QMetaObject::invokeMethod(m_thread, "calculateSpectrum",
		                                         Qt::AutoConnection,
		                                         Q_ARG(QByteArray, buffer),
		                                         Q_ARG(       int, format.sampleRate()),
		                                         Q_ARG(       int, bytesPerSample),
		                                         Q_ARG(     qreal, multiplier));
		Q_ASSERT(b);
		Q_UNUSED(b) // suppress warnings in release builds

#ifdef DUMP_SPECTRUMANALYSER
		m_textStream << "FrequencySpectrum " << m_count << "\n";
		FrequencySpectrum::const_iterator x = m_spectrum.begin();

		for (int i = 0; i < m_numSamples; ++i, ++x)
		{
			m_textStream << i << "\t"
			             << x->frequency << "\t"
			             << x->amplitude << "\t"
			             << x->phase << "\n";
		}

#endif
	}
}

bool SpectrumAnalyser::isReady() const
{
	return Idle == m_state;
}

void SpectrumAnalyser::cancelCalculation()
{
	if (Busy == m_state)
	{
		m_state = Cancelled;
	}
}

// -----------------------------------------------------------------------------
// Private slots
// -----------------------------------------------------------------------------

void SpectrumAnalyser::calculationComplete(const FrequencySpectrum &spectrum)
{
	Q_ASSERT(Idle != m_state);

	if (Busy == m_state)
	{
		emit spectrumChanged(spectrum);
	}

	m_state = Idle;
}