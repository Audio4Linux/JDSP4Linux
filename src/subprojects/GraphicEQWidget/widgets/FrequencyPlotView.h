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

#pragma once

#include <QGraphicsView>

#include "FrequencyPlotScene.h"
#include "FrequencyPlotHRuler.h"
#include "FrequencyPlotVRuler.h"

class FrequencyPlotView : public QGraphicsView
{
	Q_OBJECT
public:
	explicit FrequencyPlotView(QWidget* parent = 0);

	FrequencyPlotScene* scene() const;
	void setScene(FrequencyPlotScene* scene);

	void drawBackground(QPainter* painter, const QRectF& drawRect) override;

	void updateHRuler();

	QPoint getLastMousePos() const;
	void setLastMousePos(QPoint pos);
	using QGraphicsView::viewportMargins;

	void zoom(int deltaX, int deltaY, int mouseX, int mouseY);
	void setScrollOffsets(int x, int y);

protected:
	void wheelEvent(QWheelEvent* event) override;
	void scrollContentsBy(int dx, int dy) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent*) override;
	void showEvent(QShowEvent* event) override;

private:
	FrequencyPlotHRuler* hRuler;
	FrequencyPlotVRuler* vRuler;
	QPoint lastMousePos;
	int presetScrollX = -1;
	int presetScrollY = -1;
};
