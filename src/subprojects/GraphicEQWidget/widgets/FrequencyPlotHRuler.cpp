/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <algorithm>
#include <cmath>
#include <QPainter>
#include <QMouseEvent>

#include "FrequencyPlotView.h"
#include "FrequencyPlotHRuler.h"
#include <QApplication>
#include <QPalette>

using namespace std;

FrequencyPlotHRuler::FrequencyPlotHRuler(QWidget* parent)
	: QWidget(parent)
{
}

void FrequencyPlotHRuler::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
	FrequencyPlotScene* s = view->scene();
	QFontMetrics metrics = painter.fontMetrics();
    painter.setPen(qApp->palette().text().color());

	QPointF topLeft = view->mapToScene(0, 0);
	QPointF bottomRight = view->mapToScene(view->viewport()->width(), view->viewport()->height());
	int offsetLeft = view->viewportMargins().left();
	double fromHz = s->xToHz(topLeft.x());
	double toHz = s->xToHz(bottomRight.x());
	const vector<double>& bands = s->getBands();
	if (bands.empty())
	{
		double hzBase = pow(10, floor(log10(fromHz)));
		fromHz = floor(fromHz / hzBase) * hzBase;
		fromHz += hzBase;
		if (round(fromHz / hzBase) >= 10)
			hzBase *= 10;
		for (double hz = fromHz; hz <= toHz;)
		{
			double x = s->hzToX(hz);
			if (x != -1)
			{
				QString text;
				if (hz < 1000)
					text = QString("%0").arg(hz);
				else
					text = QString("%0k").arg(hz / 1000);
                if (metrics.horizontalAdvance(text) + 2 < s->hzToX(hz + hzBase) - x)
					painter.drawText(x - topLeft.x() + offsetLeft + 1, 0, 0, height(), Qt::TextDontClip | Qt::AlignCenter, text);
			}

			hz += hzBase;
			if (round(hz / hzBase) >= 10)
				hzBase *= 10;
		}
	}
	else
	{
		vector<double>::const_iterator it = lower_bound(bands.cbegin(), bands.cend(), fromHz);
		for (; it != bands.cend() && *it < toHz; it++)
		{
			double hz = *it;
			double x = s->hzToX(hz);
			if (x != -1)
			{
				QString text;
				if (hz < 1000)
					text = QString("%0").arg(hz);
				else
					text = QString("%0k").arg(hz / 1000);
				painter.drawText(x - topLeft.x() + offsetLeft + 1, 0, 0, height(), Qt::TextDontClip | Qt::AlignCenter, text);
			}
		}
	}

	QPoint mousePos = view->getLastMousePos();
	if (!mousePos.isNull())
	{
		QPointF mouseScenePos = view->mapToScene(mousePos);

		double hz = s->xToHz(mouseScenePos.x());
		double x = s->hzToX(hz);
		if (x != -1)
		{
			QString text = QString("%0").arg(hz, 0, 'f', 1);
			QFontMetrics metrics = painter.fontMetrics();
			QRectF rect = metrics.boundingRect(text);
			float center = x - topLeft.x() + offsetLeft + 1;
			rect = QRectF(center - ceil(rect.width() / 2) - 2 + 0.5, ceil(height() / 2) - ceil(rect.height() / 2) + 1.5, rect.width() + 3, rect.height());
			QPainterPath path;
			path.addRect(rect);
			QPainterPath topTriangle;
			topTriangle.moveTo(QPoint(center - 3, rect.top() + 1));
			topTriangle.lineTo(QPoint(center + 3, rect.top() + 1));
			topTriangle.lineTo(QPoint(center, rect.top() - 3));
			path = path.united(topTriangle);

			painter.setPen(Qt::black);
			painter.setBrush(Qt::white);
			painter.drawPath(path);
			painter.setPen(Qt::blue);
			painter.drawText(center, 0, 0, height(), Qt::TextDontClip | Qt::AlignCenter, text);
		}
	}
}

void FrequencyPlotHRuler::wheelEvent(QWheelEvent* event)
{
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    view->zoom(event->angleDelta().y(), 0, event->x() - view->viewportMargins().left(), 0);
#else
    view->zoom(event->angleDelta().y(), 0, event->position().x() - view->viewportMargins().left(), 0);
#endif
}

void FrequencyPlotHRuler::mouseMoveEvent(QMouseEvent* event)
{
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
	view->setLastMousePos(QPoint(event->x() - view->viewportMargins().left(), view->viewport()->height() - 1));
}
