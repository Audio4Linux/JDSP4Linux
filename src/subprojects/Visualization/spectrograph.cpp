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

#include "spectrograph.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimerEvent>

const int NullTimerId          = -1;
const int NullIndex            = -1;
const int BarSelectionInterval = 2000;

Spectrograph::Spectrograph(QWidget *parent)
	:   QWidget(parent)
	,   m_barSelected(NullIndex)
	,   m_timerId(NullTimerId)
	,   m_lowFreq(0.0)
	,   m_highFreq(0.0)
{
	setMinimumHeight(100);
}

Spectrograph::~Spectrograph()
{}

void Spectrograph::setParams(int   numBars,
                             qreal lowFreq,
                             qreal highFreq)
{
	Q_ASSERT( numBars > 0);
	Q_ASSERT(highFreq > lowFreq);
	m_bars.resize(numBars);
	m_lowFreq  = lowFreq;
	m_highFreq = highFreq;
	updateBars();
}

void Spectrograph::setTheme(QColor background,
                            QColor bar,
                            QColor bar_max,
                            QColor outline,
                            bool   grid,
                            Mode   mode)
{
	m_background = background;
	m_bar        = bar;
	m_bar_max    = bar_max;
	m_grid       = grid;
	m_outline    = outline;
	m_mode       = mode;
}

void Spectrograph::timerEvent(QTimerEvent *event)
{
	Q_ASSERT(event->timerId() == m_timerId);
	Q_UNUSED(event) // suppress warnings in release builds
	killTimer(m_timerId);
	m_timerId     = NullTimerId;
	m_barSelected = NullIndex;
	update();
}

void Spectrograph::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter  painter(this);
	painter.fillRect(rect(), m_background);

	const int numBars = m_bars.count();

	// Highlight region of selected bar
	if (m_barSelected != NullIndex && numBars)
	{
		QRect  regionRect = rect();
		regionRect.setLeft(m_barSelected * rect().width() / numBars);
		regionRect.setWidth(rect().width() / numBars);
		QColor regionColor(202, 202, 64);
		painter.setBrush(Qt::DiagCrossPattern);
		painter.fillRect(regionRect, regionColor);
		painter.setBrush(Qt::NoBrush);
	}

	// Draw the outline
	const QColor outlineColor = m_outline;
	QPen         outlinePen(outlineColor);
	painter.setPen(outlinePen);
	/*painter.drawLine(rect().topLeft(), rect().topRight());
	   painter.drawLine(rect().topRight(), rect().bottomRight());
	   painter.drawLine(rect().bottomRight(), rect().bottomLeft());
	   painter.drawLine(rect().bottomLeft(), rect().topLeft());*/

	const QColor   gridColor = m_bar.darker();
	QPen           gridPen(gridColor);

	QVector<qreal> dashes;
	dashes << 2 << 2;
	gridPen.setDashPattern(dashes);
	painter.setPen(gridPen);

	if (m_grid)
	{
		// Draw vertical lines between bars
		if (numBars)
		{
			const int numHorizontalSections = numBars;
			QLine     line(rect().topLeft(), rect().bottomLeft());

			for (int i = 1; i < numHorizontalSections; ++i)
			{
				line.translate(rect().width() / numHorizontalSections, 0);
				painter.drawLine(line);
			}
		}

		// Draw horizontal lines
		const int numVerticalSections = 10;
		QLine     line(rect().topLeft(), rect().topRight());

		for (int i = 1; i < numVerticalSections; ++i)
		{
			line.translate(0, rect().height() / numVerticalSections);
			painter.drawLine(line);
		}
	}

	// Draw the bars
	if (numBars)
	{
		QColor       m_bar_l = m_bar;
		m_bar_l.setAlphaF(0.75);
		m_bar_max.setAlphaF(0.75);

		QPainterPath frequencyResponse;
		painter.setRenderHint(QPainter::RenderHint::Antialiasing, true);

		// Calculate width of bars and gaps
		const int    widgetWidth      = rect().width();
		const int    barPlusGapWidth  = widgetWidth / numBars;
		const int    barWidth         = 0.8 * barPlusGapWidth;
		const int    gapWidth         = barPlusGapWidth - barWidth;
		const int    paddingWidth     = widgetWidth - numBars * (barWidth + gapWidth);
		const int    leftPaddingWidth = (paddingWidth + gapWidth) / 2;
		const int    barHeight        = rect().height() - 2 * gapWidth;

		for (int i = 0; i < numBars; ++i)
		{
			const qreal value = m_bars[i].value;
			Q_ASSERT(value >= 0.0 && value <= 1.0);
			QRect       bar   = rect();
			bar.setLeft(rect().left() + leftPaddingWidth + (i * (gapWidth + barWidth)));
			bar.setWidth(barWidth);
			bar.setTop(rect().top() + gapWidth + (1.0 - value) * barHeight);
			bar.setBottom(rect().bottom() - gapWidth);

			QColor color = m_bar_l;

			if (m_bars[i].clipped)
			{
				color = m_bar_max;
			}

			if (i == 0)
			{
				frequencyResponse.moveTo(bar.topLeft().x(),
				                         bar.topLeft().y() + 1);
			}
			else
			{
				frequencyResponse.lineTo(bar.topLeft().x(),
				                         bar.topLeft().y() + 1);
			}

			if (m_mode == Mode::Bars)
			{
				painter.fillRect(bar, color);
			}
		}

		if (m_mode == Mode::LineGradient)
		{
			frequencyResponse.lineTo(width(), height());
			frequencyResponse.lineTo(    0,   height());
			QColor          light = m_bar_l;
			light.setAlpha(60);
			QLinearGradient gradient(QPoint(width(), 0), QPoint(width(), height()));
			gradient.setColorAt(0.0, m_bar_l);
			gradient.setColorAt(1.0, light);
			painter.setBrush(gradient);
			painter.setPen(QPen(Qt::PenStyle::NoPen));
			painter.drawPath(frequencyResponse);
		}

		if (m_mode == Mode::Line)
		{
			painter.setPen(QPen(QBrush(m_bar_l), 1, Qt::PenStyle::SolidLine, Qt::PenCapStyle::SquareCap));
			painter.drawPath(frequencyResponse);
		}
	}
}

void Spectrograph::mousePressEvent(QMouseEvent *event)
{
	const QPoint pos   = event->pos();
	const int    index = m_bars.count() * (pos.x() - rect().left()) / rect().width();
	selectBar(index);
}

void Spectrograph::reset()
{
	m_spectrum.reset();
	spectrumChanged(m_spectrum);
}

void Spectrograph::spectrumChanged(const FrequencySpectrum &spectrum)
{
	m_spectrum = spectrum;
	updateBars();
}

int Spectrograph::barIndex(qreal frequency) const
{
	Q_ASSERT(frequency >= m_lowFreq && frequency < m_highFreq);
	const qreal bandWidth = (m_highFreq - m_lowFreq) / m_bars.count();
	const int   index     = (frequency - m_lowFreq) / bandWidth;

	if (index < 0 || index >= m_bars.count())
	{
		Q_ASSERT(false);
	}

	return index;
}

QPair<qreal, qreal> Spectrograph::barRange(int index) const
{
	Q_ASSERT(index >= 0 && index < m_bars.count());
	const qreal bandWidth = (m_highFreq - m_lowFreq) / m_bars.count();
	return QPair<qreal, qreal>(index * bandWidth, (index + 1) * bandWidth);
}

void Spectrograph::updateBars()
{
	m_bars.fill(Bar());
	FrequencySpectrum::const_iterator       i   = m_spectrum.begin();
	const FrequencySpectrum::const_iterator end = m_spectrum.end();

	for (; i != end; ++i)
	{
		const FrequencySpectrum::Element e = *i;

		if (e.frequency >= m_lowFreq && e.frequency < m_highFreq)
		{
			Bar &bar = m_bars[barIndex(e.frequency)];
			bar.value    = qMax(bar.value, e.amplitude);
			bar.clipped |= e.clipped;
		}
	}

	update();
}

void Spectrograph::selectBar(int index)
{
	Q_UNUSED(index)
	/*const QPair<qreal, qreal> frequencyRange = barRange(index);
	   const QString message = QString("%1 - %2 Hz")
	                            .arg(frequencyRange.first)
	                            .arg(frequencyRange.second);
	   emit infoMessage(message, BarSelectionInterval);

	   if (NullTimerId != m_timerId)
	    killTimer(m_timerId);
	   m_timerId = startTimer(BarSelectionInterval);

	   m_barSelected = index;
	   update();*/
}