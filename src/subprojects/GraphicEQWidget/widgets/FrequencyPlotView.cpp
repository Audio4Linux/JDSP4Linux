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
#include <QWheelEvent>
#include <QScrollBar>

#include "helpers/DPIHelper.h"
#include "FrequencyPlotItem.h"
#include "FrequencyPlotView.h"

using namespace std;

FrequencyPlotView::FrequencyPlotView(QWidget* parent)
	: QGraphicsView(parent)
{
	setViewportMargins(DPIHelper::scale(32), 0, 0, DPIHelper::scale(20));
	hRuler = new FrequencyPlotHRuler(this);
	vRuler = new FrequencyPlotVRuler(this);
	hRuler->setMouseTracking(true);
	vRuler->setMouseTracking(true);
}

FrequencyPlotScene* FrequencyPlotView::scene() const
{
	return qobject_cast<FrequencyPlotScene*>(QGraphicsView::scene());
}

void FrequencyPlotView::setScene(FrequencyPlotScene* scene)
{
	QGraphicsView::setScene(scene);
}

void FrequencyPlotView::drawBackground(QPainter* painter, const QRectF& drawRect)
{
	painter->setRenderHint(QPainter::Antialiasing, false);

	QRectF rect = drawRect;
	if (!sceneRect().contains(rect))
	{
		rect = rect.intersected(sceneRect());
		painter->setClipRect(rect);
	}

	FrequencyPlotScene* s = scene();
	QPointF topLeft = mapToScene(0, 0);
	QPointF bottomRight = mapToScene(viewport()->width(), viewport()->height());
	double dbStep = abs(s->yToDb(0) - s->yToDb(DPIHelper::scale(30)));

	double dbBase = pow(10, floor(log10(dbStep)));
	if (dbStep >= 5 * dbBase)
		dbStep = 5 * dbBase;
	else if (dbStep >= 2 * dbBase)
		dbStep = 2 * dbBase;
	else
		dbStep = dbBase;

	double fromDb = floor(s->yToDb(rect.top() + rect.height()) / dbStep) * dbStep;
	double toDb = ceil(s->yToDb(rect.top()) / dbStep) * dbStep;

    painter->setPen(palette().window().color());
	for (double db = fromDb; db <= toDb; db += dbStep)
	{
		double y = floor(s->dbToY(db)) + 0.5;
		if (y != -1)
			painter->drawLine(topLeft.x(), y, bottomRight.x(), y);
	}

	double fromHz = s->xToHz(rect.left());
	double toHz = s->xToHz(rect.left() + rect.width());

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
			double x = floor(s->hzToX(hz)) + 0.5;
			if (x >= 0)
				painter->drawLine(x, topLeft.y(), x, bottomRight.y());

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
			double x = floor(s->hzToX(hz)) + 0.5;
			if (x >= 0)
				painter->drawLine(x, topLeft.y(), x, bottomRight.y());
		}
	}
}

void FrequencyPlotView::updateHRuler()
{
	hRuler->update();
}

QPoint FrequencyPlotView::getLastMousePos() const
{
	return lastMousePos;
}

void FrequencyPlotView::setLastMousePos(QPoint pos)
{
	lastMousePos = pos;
	hRuler->update();
	vRuler->update();
}

void FrequencyPlotView::zoom(int deltaX, int deltaY, int mouseX, int mouseY)
{
	FrequencyPlotScene* s = scene();
	QPointF scenePos = mapToScene(mouseX, mouseY);
	double hz = s->xToHz(scenePos.x());
	double db = s->yToDb(scenePos.y());

	qreal zoomFactorX = pow(1.001, deltaX);
	qreal zoomFactorY = pow(1.001, deltaY);
	double zoomX = max(0.5, min(30.0, s->getZoomX() * zoomFactorX));
	double zoomY = max(0.5, min(30.0, s->getZoomY() * zoomFactorY));
	s->setZoom(zoomX, zoomY);

	if (deltaX != 0)
	{
		double x = s->hzToX(hz);
		if (x != -1)
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() + round(x - scenePos.x()));
	}

	if (deltaY != 0)
	{
		double y = s->dbToY(db);
		if (y != -1)
			verticalScrollBar()->setValue(verticalScrollBar()->value() + round(y - scenePos.y()));
	}

	resetCachedContent();
	viewport()->update();
	if (deltaX != 0)
		hRuler->update();
	if (deltaY != 0)
		vRuler->update();
}

void FrequencyPlotView::setScrollOffsets(int x, int y)
{
	presetScrollX = x;
	presetScrollY = y;
}

void FrequencyPlotView::wheelEvent(QWheelEvent* event)
{
	event->accept();
	int delta = event->angleDelta().y();
    zoom(delta, delta, event->position().x(), event->position().y());
}

void FrequencyPlotView::scrollContentsBy(int dx, int dy)
{
	QGraphicsView::scrollContentsBy(dx, dy);

	if (dx != 0)
		hRuler->update();
	if (dy != 0)
		vRuler->update();
}

void FrequencyPlotView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);

	const QRect rect = viewport()->geometry();
	QMargins margins = viewportMargins();
	hRuler->setGeometry(rect.x() - margins.left(), rect.y() + rect.height(), rect.width() + margins.left(), margins.bottom());
	vRuler->setGeometry(rect.x() - margins.left(), rect.y(), margins.left(), rect.height() + margins.bottom());
}

void FrequencyPlotView::mousePressEvent(QMouseEvent* event)
{
	QGraphicsView::mousePressEvent(event);

	setLastMousePos(event->pos());
}

void FrequencyPlotView::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::RightButton)
	{
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - lastMousePos.x()));
		verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - lastMousePos.y()));
	}
	else
	{
		QGraphicsView::mouseMoveEvent(event);
	}
	setLastMousePos(event->pos());
}

void FrequencyPlotView::leaveEvent(QEvent*)
{
	setLastMousePos(QPoint());
}

void FrequencyPlotView::showEvent(QShowEvent* event)
{
	QGraphicsView::showEvent(event);

	if (presetScrollX != -1)
	{
		horizontalScrollBar()->setValue(presetScrollX);
		presetScrollX = -1;
	}

	if (presetScrollY != -1)
	{
		verticalScrollBar()->setValue(presetScrollY);
		presetScrollY = -1;
	}
}
