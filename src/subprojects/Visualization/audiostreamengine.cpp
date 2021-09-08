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

#include "audiostreamengine.h"
#include "utils.h"

#include <math.h>

#include <QAudioInput>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMetaObject>
#include <QSet>
#include <QThread>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

const qint64 BufferDurationUs = 10 * 100000000;

// Size of the level calculation window in microseconds
const int    LevelWindowUs    = 0.1 * 100;

// -----------------------------------------------------------------------------
// Constructor and destructor
// -----------------------------------------------------------------------------

AudioStreamEngine::AudioStreamEngine(QObject *parent)
	:   QObject(parent)
	,   m_mode(QAudio::AudioInput)
	,   m_state(QAudio::StoppedState)
	,   m_availableAudioInputDevices
	    (QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
	,   m_audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
	,   m_audioInput(0)
	,   m_audioInputIODevice(0)
	,   m_recordPosition(0)
	,   m_availableAudioOutputDevices
	    (QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
	,   m_audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
	,   m_playPosition(0)
	,   m_bufferPosition(0)
	,   m_bufferLength(0)
	,   m_dataLength(0)
	,   m_levelBufferLength(0)
	,   m_rmsLevel(0.0)
	,   m_peakLevel(0.0)
	,   m_spectrumBufferLength(0)
	,   m_spectrumAnalyser()
	,   m_spectrumPosition(0)
	,   m_count(0)
{
	qRegisterMetaType<FrequencySpectrum>("FrequencySpectrum");
	qRegisterMetaType<WindowFunction>(   "WindowFunction");
	CHECKED_CONNECT(&m_spectrumAnalyser,
	                SIGNAL(spectrumChanged(FrequencySpectrum)),
	                this,
	                SLOT(spectrumChanged(FrequencySpectrum)));

	initialize();

#ifdef DUMP_DATA
	createOutputDir();
#endif

#ifdef DUMP_SPECTRUM
	m_spectrumAnalyser.setOutputPath(outputPath());
#endif
}

AudioStreamEngine::~AudioStreamEngine()
{}

// -----------------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------------

bool AudioStreamEngine::initializeRecord()
{
	reset();
	ENGINE_DEBUG << "Engine::initializeRecord";
	m_tone = SweptTone();
	return initialize();
}

void AudioStreamEngine::setWindowFunction(WindowFunction type)
{
	m_spectrumAnalyser.setWindowFunction(type);
}

void AudioStreamEngine::setMultiplier(float m)
{
	m_multiplier = m;
}

void AudioStreamEngine::setNotifyIntervalMs(int ms)
{
	NotifyIntervalMs = ms;
}

qint64 AudioStreamEngine::bufferLength() const
{
	return m_bufferLength;
}

// -----------------------------------------------------------------------------
// Public slots
// -----------------------------------------------------------------------------

void AudioStreamEngine::startRecording()
{
	if (m_audioInput)
	{
		if (QAudio::AudioInput == m_mode &&
		    QAudio::SuspendedState == m_state)
		{
			m_audioInput->resume();
		}
		else
		{
			m_spectrumAnalyser.cancelCalculation();
			spectrumChanged(0, 0, FrequencySpectrum());

			m_buffer.fill(0);
			setRecordPosition(0, true);

			m_mode = QAudio::AudioInput;
			CHECKED_CONNECT(m_audioInput, SIGNAL(stateChanged(QAudio::State)),
			                this, SLOT(audioStateChanged(QAudio::State)));
			CHECKED_CONNECT(m_audioInput, SIGNAL(notify()),
			                this, SLOT(audioNotify()));
			m_count              = 0;
			m_dataLength         = 0;
			emit dataLengthChanged(0);
			m_audioInputIODevice = m_audioInput->start();
			CHECKED_CONNECT(m_audioInputIODevice, SIGNAL(readyRead()),
			                this, SLOT(audioDataReady()));
		}
	}
}

void AudioStreamEngine::setAudioInputDevice(const QAudioDeviceInfo &device)
{
	if (device.deviceName() != m_audioInputDevice.deviceName())
	{
		m_audioInputDevice = device;
		initialize();
	}
}

void AudioStreamEngine::setAudioOutputDevice(const QAudioDeviceInfo &device)
{
	if (device.deviceName() != m_audioOutputDevice.deviceName())
	{
		m_audioOutputDevice = device;
		initialize();
	}
}

// -----------------------------------------------------------------------------
// Private slots
// -----------------------------------------------------------------------------

void AudioStreamEngine::audioNotify()
{
	switch (m_mode)
	{
		case QAudio::AudioInput:
		{
			const qint64 recordPosition = qMin(m_bufferLength, audioLength(m_format, m_audioInput->processedUSecs()));
			setRecordPosition(recordPosition);
			const qint64 levelPosition  = m_dataLength - m_levelBufferLength;

			if (levelPosition >= 0)
			{
				calculateLevel(levelPosition, m_levelBufferLength);
			}

			if (m_dataLength >= m_spectrumBufferLength)
			{
				const qint64 spectrumPosition = m_dataLength - m_spectrumBufferLength;
				calculateSpectrum(spectrumPosition);
			}

			emit bufferChanged(0, m_dataLength, m_buffer);
		}
		case QAudio::AudioOutput:
			break;
	}
}

void AudioStreamEngine::audioStateChanged(QAudio::State state)
{
	ENGINE_DEBUG << "Engine::audioStateChanged from" << m_state
	             << "to" << state;

	if (QAudio::StoppedState == state)
	{
		// Check error
		QAudio::Error error = QAudio::NoError;

		switch (m_mode)
		{
			case QAudio::AudioInput:
				error = m_audioInput->error();
				break;
			default:

				if (QAudio::NoError != error)
				{
					reset();
					return;
				}
		}
	}

	setState(state);
}

void AudioStreamEngine::audioDataReady()
{
	Q_ASSERT(0 == m_bufferPosition);

	const qint64 bytesReady  = m_audioInput->bytesReady();
	const qint64 bytesSpace  = m_buffer.size() - m_dataLength;
	const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

	const qint64 bytesRead   = m_audioInputIODevice->read(
		m_buffer.data() + m_dataLength,
		bytesToRead);

	if (bytesRead)
	{
		m_dataLength += bytesRead;
		emit dataLengthChanged(dataLength());
	}

	if (m_buffer.size() == m_dataLength)
	{
		m_buffer.fill(0);
		m_dataLength = 0;
	}
}

void AudioStreamEngine::spectrumChanged(const FrequencySpectrum &spectrum)
{
	ENGINE_DEBUG << "Engine::spectrumChanged" << "pos" << m_spectrumPosition;
	emit spectrumChanged(m_spectrumPosition, m_spectrumBufferLength, spectrum);
}

// -----------------------------------------------------------------------------
// Private functions
// -----------------------------------------------------------------------------

void AudioStreamEngine::resetAudioDevices()
{
	delete m_audioInput;
	m_audioInput         = 0;
	m_audioInputIODevice = 0;
	setRecordPosition(0);
	setPlayPosition(0);
	m_spectrumPosition   = 0;
	setLevel(0.0, 0.0, 0);
}

void AudioStreamEngine::reset()
{
	stopRecording();
	setState(QAudio::AudioInput, QAudio::StoppedState);
	setFormat(QAudioFormat());
	m_buffer.clear();
	m_bufferPosition = 0;
	m_bufferLength   = 0;
	m_dataLength     = 0;
	emit dataLengthChanged(0);
	resetAudioDevices();
}

bool AudioStreamEngine::initialize()
{
	bool         result = false;

	QAudioFormat format = m_format;

	if (selectFormat())
	{
		if (m_format != format)
		{
			resetAudioDevices();
			m_bufferLength = audioLength(m_format, BufferDurationUs);
			m_buffer.resize(m_bufferLength);
			m_buffer.fill(0);
			emit bufferLengthChanged(bufferLength());
			emit bufferChanged(0, 0, m_buffer);
			m_audioInput = new QAudioInput(m_audioInputDevice, m_format, this);
			m_audioInput->setNotifyInterval(NotifyIntervalMs);
			result       = true;
		}
	}
	else
	{
		emit errorMessage(tr("No common input / output format found"), "");
	}

	ENGINE_DEBUG << "Engine::initialize" << "m_bufferLength" << m_bufferLength;
	ENGINE_DEBUG << "Engine::initialize" << "m_dataLength" << m_dataLength;
	ENGINE_DEBUG << "Engine::initialize" << "format" << m_format;

	return result;
}

bool AudioStreamEngine::selectFormat()
{
	bool foundSupportedFormat = false;

	if (QAudioFormat() != m_format)
	{
		QAudioFormat format = m_format;

		if (m_audioOutputDevice.isFormatSupported(format))
		{
			setFormat(format);
			foundSupportedFormat = true;
		}
	}
	else
	{
		QList<int> sampleRatesList;
#ifdef Q_OS_WIN
		// The Windows audio backend does not correctly report format support
		// (see QTBUG-9100).  Furthermore, although the audio subsystem captures
		// at 11025Hz, the resulting audio is corrupted.
		sampleRatesList += 8000;
#endif

		sampleRatesList += m_audioInputDevice.supportedSampleRates();

		sampleRatesList += m_audioOutputDevice.supportedSampleRates();
		sampleRatesList  = sampleRatesList.toSet().toList(); // remove duplicates
		std::sort(sampleRatesList.begin(), sampleRatesList.end());
		ENGINE_DEBUG << "Engine::initialize frequenciesList" << sampleRatesList;

		QList<int> channelsList;
		channelsList += m_audioInputDevice.supportedChannelCounts();
		channelsList += m_audioOutputDevice.supportedChannelCounts();
		channelsList  = channelsList.toSet().toList();
		std::sort(channelsList.begin(), channelsList.end());
		ENGINE_DEBUG << "Engine::initialize channelsList" << channelsList;

		QAudioFormat format;
		format.setByteOrder(QAudioFormat::LittleEndian);
		format.setCodec("audio/pcm");
		format.setSampleSize(16);
		format.setSampleType(QAudioFormat::SignedInt);
		int          sampleRate, channels;
		foreach(sampleRate, sampleRatesList)
		{
			if (foundSupportedFormat)
			{
				break;
			}

			format.setSampleRate(sampleRate);
			foreach(channels, channelsList)
			{
				format.setChannelCount(channels);
				const bool inputSupport  = m_audioInputDevice.isFormatSupported(format);
				const bool outputSupport = m_audioOutputDevice.isFormatSupported(format);
				ENGINE_DEBUG << "Engine::initialize checking " << format
				             << "input" << inputSupport
				             << "output" << outputSupport;

				if (inputSupport && outputSupport)
				{
					foundSupportedFormat = true;
					break;
				}
			}
		}

		if (!foundSupportedFormat)
		{
			format = QAudioFormat();
		}

		setFormat(format);
	}

	return foundSupportedFormat;
}

void AudioStreamEngine::stopRecording()
{
	if (m_audioInput)
	{
		m_audioInput->stop();
		QCoreApplication::instance()->processEvents();
		m_audioInput->disconnect();
	}

	m_audioInputIODevice = 0;
#ifdef DUMP_AUDIO
	dumpData();
#endif
}

void AudioStreamEngine::setState(QAudio::State state)
{
	const bool changed = (m_state != state);
	m_state = state;

	if (changed)
	{
		emit stateChanged(m_mode, m_state);
	}
}

void AudioStreamEngine::setState(QAudio::Mode  mode,
                                 QAudio::State state)
{
	const bool changed = (m_mode != mode || m_state != state);
	m_mode  = mode;
	m_state = state;

	if (changed)
	{
		emit stateChanged(m_mode, m_state);
	}
}

void AudioStreamEngine::setRecordPosition(qint64 position,
                                          bool   forceEmit)
{
	const bool changed = (m_recordPosition != position);
	m_recordPosition = position;

	if (changed || forceEmit)
	{
		emit recordPositionChanged(m_recordPosition);
	}
}

void AudioStreamEngine::setPlayPosition(qint64 position,
                                        bool   forceEmit)
{
	const bool changed = (m_playPosition != position);
	m_playPosition = position;

	if (changed || forceEmit)
	{
		emit playPositionChanged(m_playPosition);
	}
}

void AudioStreamEngine::calculateLevel(qint64 position,
                                       qint64 length)
{
#ifdef DISABLE_LEVEL
	Q_UNUSED(position)
	Q_UNUSED(length)
#else
	Q_ASSERT(position + length <= m_bufferPosition + m_dataLength);

	qreal            peakLevel = 0.0;

	qreal            sum       = 0.0;
	const char      *ptr       = m_buffer.constData() + position - m_bufferPosition;
	const char*const end       = ptr + length;

	while (ptr < end)
	{
		const qint16 value     = *reinterpret_cast<const qint16*>(ptr);
		const qreal  fracValue = pcmToReal(value);
		peakLevel = qMax(peakLevel, fracValue);
		sum      += fracValue * fracValue;
		ptr      += 2;
	}

	const int numSamples = length / 2;
	qreal     rmsLevel   = sqrt(sum / numSamples);

	rmsLevel = qMax(qreal(0.0), rmsLevel);
	rmsLevel = qMin(qreal(1.0), rmsLevel);
	setLevel(rmsLevel, peakLevel, numSamples);

	ENGINE_DEBUG << "Engine::calculateLevel" << "pos" << position << "len" << length
	             << "rms" << rmsLevel << "peak" << peakLevel;
#endif
}

void AudioStreamEngine::calculateSpectrum(qint64 position)
{
#ifdef DISABLE_SPECTRUM
	Q_UNUSED(position)
#else
	Q_ASSERT(position + m_spectrumBufferLength <= m_bufferPosition + m_dataLength);
	Q_ASSERT(0 == m_spectrumBufferLength % 2); // constraint of FFT algorithm

	// QThread::currentThread is marked 'for internal use only', but
	// we're only using it for debug output here, so it's probably OK :)
	ENGINE_DEBUG << "Engine::calculateSpectrum" << QThread::currentThread()
	             << "count" << m_count << "pos" << position << "len" << m_spectrumBufferLength
	             << "spectrumAnalyser.isReady" << m_spectrumAnalyser.isReady();

	if (m_spectrumAnalyser.isReady())
	{
		m_spectrumBuffer   = QByteArray::fromRawData(m_buffer.constData() + position - m_bufferPosition,
		                                             m_spectrumBufferLength);
		m_spectrumPosition = position;
		m_spectrumAnalyser.calculate(m_spectrumBuffer, m_format, (qreal) m_multiplier);
	}

#endif
}

void AudioStreamEngine::setFormat(const QAudioFormat &format)
{
	const bool changed = (format != m_format);
	m_format               = format;
	m_levelBufferLength    = audioLength(m_format, LevelWindowUs);
	m_spectrumBufferLength = SpectrumLengthSamples *
	                         (m_format.sampleSize() / 8) * m_format.channelCount();

	if (changed)
	{
		emit formatChanged(m_format);
	}
}

void AudioStreamEngine::setLevel(qreal rmsLevel,
                                 qreal peakLevel,
                                 int   numSamples)
{
	m_rmsLevel  = rmsLevel;
	m_peakLevel = peakLevel;
	emit levelChanged(m_rmsLevel, m_peakLevel, numSamples);
}

#ifdef DUMP_DATA
void Engine::createOutputDir()
{
	m_outputDir.setPath("output");

	// Ensure output directory exists and is empty
	if (m_outputDir.exists())
	{
		const QStringList files = m_outputDir.entryList(QDir::Files);
		QString           file;
		foreach(file, files)
		m_outputDir.remove(file);
	}
	else
	{
		QDir::current().mkdir("output");
	}
}

#endif // DUMP_DATA

#ifdef DUMP_AUDIO
void Engine::dumpData()
{
	const QString txtFileName = m_outputDir.filePath("data.txt");
	QFile         txtFile(txtFileName);
	txtFile.open(QFile::WriteOnly | QFile::Text);
	QTextStream   stream(&txtFile);
	const qint16 *ptr        = reinterpret_cast<const qint16*>(m_buffer.constData());
	const int     numSamples = m_dataLength / (2 * m_format.channels());

	for (int i = 0; i < numSamples; ++i)
	{
		stream << i << "\t" << *ptr << "\n";
		ptr += m_format.channels();
	}

	const QString pcmFileName = m_outputDir.filePath("data.pcm");
	QFile         pcmFile(pcmFileName);
	pcmFile.open(QFile::WriteOnly);
	pcmFile.write(m_buffer.constData(), m_dataLength);
}

#endif // DUMP_AUDIO