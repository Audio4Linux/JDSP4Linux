/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2016  Jonas Thedering

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

#include "DPIHelper.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QScreen>

QSize DPIHelper::scale(QSize size)
{
	if (qApp->testAttribute(Qt::AA_Use96Dpi))
		return size;

	qreal dpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();
	return QSize(qRound(size.width() * dpi / 96), qRound(size.height() * dpi / 96));
}

int DPIHelper::scale(double pixel)
{
	if (qApp->testAttribute(Qt::AA_Use96Dpi))
		return qRound(pixel);

	qreal dpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();
	return qRound(pixel * dpi / 96);
}

double DPIHelper::scaleZoom(double zoom)
{
	if (qApp->testAttribute(Qt::AA_Use96Dpi))
		return zoom;

	qreal dpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();
	return zoom * dpi / 96;
}

double DPIHelper::invScale(int pixel)
{
	if (qApp->testAttribute(Qt::AA_Use96Dpi))
		return pixel;

	qreal dpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();
	return pixel * 96 / dpi;
}

double DPIHelper::invScaleZoom(double zoom)
{
	if (qApp->testAttribute(Qt::AA_Use96Dpi))
		return zoom;

	qreal dpi = QGuiApplication::primaryScreen()->logicalDotsPerInchX();
	return zoom * 96 / dpi;
}
