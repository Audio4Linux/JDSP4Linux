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

#include "GraphicEQFilterGUIScene.h"
#include "GraphicEQFilterGUIView.h"
#include <cmath>
#include <colorprovider.h>

using namespace ColorProvider;

GraphicEQFilterGUIView::GraphicEQFilterGUIView(QWidget* parent)
	: FrequencyPlotView(parent)
{
}

void GraphicEQFilterGUIView::drawBackground(QPainter* painter, const QRectF& rect)
{
	FrequencyPlotView::drawBackground(painter, rect);

	painter->setRenderHint(QPainter::Antialiasing, true);
	GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
	std::vector<FilterNode>& nodes = s->getNodes();
	GainIterator gainIterator(nodes);
	QPainterPath path;
	bool first = true;
	double lastDb = -1000;
	for (int x = rect.left() - 1; x <= rect.right() + 1; x++)
	{
		double hz = s->xToHz(x);
		double db = gainIterator.gainAt(hz);
		double y = s->dbToY(db);
		if (db == lastDb)
			y = floor(y) + 0.5;
		lastDb = db;
		if (first)
		{
			path.moveTo(x, y);
			first = false;
		}
		else
		{
			path.lineTo(x, y);
		}
	}

    Hsl hsl0 = Hsl::ofQColor(palette().color(QPalette::WindowText));
    Hsl hsl1 = Hsl::ofQColor(palette().color(QPalette::Window));
    bool isLightPalette = hsl0.l < hsl1.l;

    painter->setPen(isLightPalette ? palette().color(QPalette::Disabled,QPalette::WindowText).darker() :
                                     palette().color(QPalette::Disabled,QPalette::WindowText).lighter());
	painter->drawPath(path);
}
