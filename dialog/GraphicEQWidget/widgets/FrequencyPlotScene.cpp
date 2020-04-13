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

#include "helpers/DPIHelper.h"
#include "FrequencyPlotView.h"
#include "FrequencyPlotScene.h"
#include <cmath>
#include <algorithm>

using namespace std;

static double maxX = 1000;
static double maxY = 500;
static double minHz = 1;
static double maxHz = 30000;
static double minDb = -30;
static double maxDb = 30;

vector<double> FrequencyPlotScene::bands15 = {25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000};
vector<double> FrequencyPlotScene::bands31 = {20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800,
											  1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000};
vector<double> FrequencyPlotScene::bandsVar;

FrequencyPlotScene::FrequencyPlotScene(QObject* parent)
	: QGraphicsScene(parent)
{
	zoomX = DPIHelper::scaleZoom(1.0);
    zoomY = DPIHelper::scaleZoom(1.0);

	updateSceneRect();
}

void FrequencyPlotScene::addItem(FrequencyPlotItem* item)
{
	QGraphicsScene::addItem(item);
}

double FrequencyPlotScene::xToHz(double x)
{
	return pow(maxHz / minHz, x / zoomX / maxX) * minHz;
}

double FrequencyPlotScene::hzToX(double hz)
{
	if (hz < minHz || hz > maxHz)
		return -1;

	return log(hz / minHz) / log(maxHz / minHz) * maxX * zoomX;
}

double FrequencyPlotScene::yToDb(double y)
{
	return(minDb + (1 - y / zoomY / maxY) * (maxDb - minDb));
}

double FrequencyPlotScene::dbToY(double db)
{
	if (db < minDb || db > maxDb)
		return -1;

	return (1 - (db - minDb) / (maxDb - minDb)) * maxY * zoomY;
}

double FrequencyPlotScene::getZoomX() const
{
	return zoomX;
}

double FrequencyPlotScene::getZoomY() const
{
	return zoomY;
}

void FrequencyPlotScene::setZoom(double zoomX, double zoomY)
{
	if (zoomX != this->zoomX || zoomY != this->zoomY)
	{
		this->zoomX = zoomX;
		this->zoomY = zoomY;
		updateSceneRect();

		for (QGraphicsItem* item : items())
		{
			FrequencyPlotItem* plotItem = (FrequencyPlotItem*)item;
			plotItem->updatePos();
		}
	}
}

void FrequencyPlotScene::updateSceneRect()
{
	setSceneRect(0, 0, maxX * zoomX, maxY * zoomY);
}

int FrequencyPlotScene::getBandCount() const
{
	return bandCount;
}

void FrequencyPlotScene::setBandCount(int value)
{
	if (value != bandCount)
	{
		bandCount = value;
		update();
		FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(views()[0]);
		view->updateHRuler();
	}
}

const vector<double>& FrequencyPlotScene::getBands()
{
	return getBands(bandCount);
}

const std::vector<double>& FrequencyPlotScene::getBands(int count)
{
	switch (count)
	{
	case 15:
		return bands15;
	case 31:
		return bands31;
	default:
		return bandsVar;
	}
}
