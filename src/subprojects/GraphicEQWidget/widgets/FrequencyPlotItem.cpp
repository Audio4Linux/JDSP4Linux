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

#include <QGraphicsScene>
#include <cmath>
#include <algorithm>
#include <QApplication>
#include <QPalette>

#include "FrequencyPlotScene.h"
#include "FrequencyPlotItem.h"

FrequencyPlotItem::FrequencyPlotItem(double hz, double db)
	: hz(hz), db(db)
{
	setFlag(ItemSendsScenePositionChanges);
}

void FrequencyPlotItem::updatePos()
{
	setFlag(ItemSendsScenePositionChanges, false);
	FrequencyPlotScene* s = qobject_cast<FrequencyPlotScene*>(scene());
	setPos(floor(s->hzToX(hz)) + 0.5, floor(s->dbToY(db)) + 0.5);
	setFlag(ItemSendsScenePositionChanges, true);
}

double FrequencyPlotItem::getHz() const
{
	return hz;
}

void FrequencyPlotItem::setHz(double value)
{
	hz = value;
}

double FrequencyPlotItem::getDb() const
{
	return db;
}

void FrequencyPlotItem::setDb(double value)
{
	db = value;
}
