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

#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <colorprovider.h>
#include "helpers/DPIHelper.h"
#include "GraphicEQFilterGUIScene.h"
#include "GraphicEQFilterGUIItem.h"

using namespace ColorProvider;

static double size = 10;

GraphicEQFilterGUIItem::GraphicEQFilterGUIItem(int index, double hz, double db)
	: FrequencyPlotItem(hz, db), index(index)
{
	setFlag(ItemIsMovable);
	setFlag(ItemIsSelectable);
}

QRectF GraphicEQFilterGUIItem::boundingRect() const
{
	int s = DPIHelper::scale(size);
	return QRectF(-s / 2, -s / 2, s, s);
}

QPainterPath GraphicEQFilterGUIItem::shape() const
{
	QPainterPath path;
	path.addEllipse(boundingRect());
	return path;
}

void GraphicEQFilterGUIItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    auto pal = qApp->palette();
    auto shape_border_light = pal.color(QPalette::Disabled,QPalette::WindowText).darker(150);
    auto shape_border_dark = pal.color(QPalette::Disabled,QPalette::Window).lighter(150);
    auto text_light = pal.color(QPalette::Active,QPalette::WindowText).darker(120);
    auto text_dark = pal.color(QPalette::Active,QPalette::WindowText).lighter(120);

    Hsl hsl0 = Hsl::ofQColor(pal.color(QPalette::WindowText));
    Hsl hsl1 = Hsl::ofQColor(pal.color(QPalette::Window));
    bool isLightPalette = hsl0.l < hsl1.l;

    painter->setBrush(qApp->palette().window());
    painter->setPen(isLightPalette ? shape_border_light : shape_border_dark);
	if (isSelected())
		painter->setBrush(QColor(38, 147, 255));
	painter->drawPath(shape());
	if (index < 99)
	{
		if (isSelected())
            painter->setPen(qApp->palette().window().color());
        else
            painter->setPen(isLightPalette ? text_light : text_dark);

		QFont font;
		font.setPixelSize(DPIHelper::scale(9));
		font.setLetterSpacing(QFont::AbsoluteSpacing, -1);
		QFontMetrics metrics(font);
		painter->setFont(font);
		QString text = QString::number(index + 1);
		painter->drawText(-metrics.width(text) / 2, metrics.boundingRect('0').height() / 2, text);
	}
}

QVariant GraphicEQFilterGUIItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
	QVariant result = QGraphicsItem::itemChange(change, value);

	if (change == ItemScenePositionHasChanged)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
		s->itemMoved(index);
	}
	else if (change == ItemSelectedHasChanged)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
		s->itemSelectionChanged(index, value.toBool());
	}
	else if (change == ItemPositionChange)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
        if (s->getBandCount() != -1 && !s->get15BandFreeMode())
		{
			QPointF newPos = value.toPointF();
			newPos.setX(s->hzToX(getHz()));
			result = newPos;
		}
	}

	return result;
}
int GraphicEQFilterGUIItem::getIndex() const
{
	return index;
}

void GraphicEQFilterGUIItem::setIndex(int value)
{
	index = value;
}
