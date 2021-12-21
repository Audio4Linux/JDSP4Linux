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

#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <QApplication>
#include <QPalette>

#include "helpers/DPIHelper.h"
#include "FrequencyPlotView.h"
#include "FrequencyPlotScene.h"
#include "FrequencyPlotVRuler.h"

FrequencyPlotVRuler::FrequencyPlotVRuler(QWidget* parent)
	: QWidget(parent)
{
}

void FrequencyPlotVRuler::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
	FrequencyPlotScene* s = view->scene();

	QPointF topLeft = view->mapToScene(0, 0);
	QPointF bottomRight = view->mapToScene(view->viewport()->width(), view->viewport()->height());
	double dbStep = abs(s->yToDb(0) - s->yToDb(DPIHelper::scale(30)));

	double dbBase = pow(10, floor(log10(dbStep)));
	if (dbStep >= 5 * dbBase)
		dbStep = 5 * dbBase;
	else if (dbStep >= 2 * dbBase)
		dbStep = 2 * dbBase;
	else
		dbStep = dbBase;

	double fromDb = floor(s->yToDb(topLeft.y()) / dbStep) * dbStep;
	double toDb = ceil(s->yToDb(bottomRight.y()) / dbStep) * dbStep;

    painter.setPen(qApp->palette().text().color());
	for (double db = toDb; db <= fromDb; db += dbStep)
	{
		if (abs(db) < 1e-6)
			db = 0;
		double y = s->dbToY(db);
		if (y != -1)
			painter.drawText(0, y - topLeft.y() - 1, width(), 0, Qt::TextDontClip | Qt::AlignCenter, QString("%0").arg(db));
	}

	QPoint mousePos = view->getLastMousePos();
	if (!mousePos.isNull())
	{
		QPointF mouseScenePos = view->mapToScene(mousePos);

		double db = s->yToDb(mouseScenePos.y());
		double y = s->dbToY(db);
		if (y != -1)
		{
			QString text = QString("%0").arg(db, 0, 'f', 1);
			QFontMetrics metrics = painter.fontMetrics();
			QRectF rect = metrics.boundingRect(text);
			float center = y - topLeft.y() - 1;
			rect = QRectF(ceil(width() / 2) - ceil(rect.width() / 2) - 2.5, center - rect.height() / 2 + 1, rect.width() + 3, rect.height());
			QPainterPath path;
			path.addRect(rect);
			QPainterPath rightTriangle;
			rightTriangle.moveTo(QPoint(rect.right(), center - 2));
			rightTriangle.lineTo(QPoint(rect.right(), center + 4));
			rightTriangle.lineTo(QPoint(rect.right() + 4, center + 1));
			path = path.united(rightTriangle);

			painter.setPen(Qt::black);
			painter.setBrush(Qt::white);
			painter.drawPath(path);
			painter.setPen(Qt::blue);
			painter.drawText(0, center, width() - 2, 0, Qt::TextDontClip | Qt::AlignCenter, text);
		}
	}
}

void FrequencyPlotVRuler::wheelEvent(QWheelEvent* event)
{
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
    view->zoom(0, event->angleDelta().y(), 0, event->position().y());
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    view->zoom(0, event->angleDelta().y(), 0, event->y());
#else
    view->zoom(0, event->angleDelta().y(), 0, event->position().y());
#endif
}

void FrequencyPlotVRuler::mouseMoveEvent(QMouseEvent* event)
{
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
    view->setLastMousePos(QPoint(0, std::min(event->y(), view->viewport()->height() - 1)));
}
