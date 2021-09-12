/*
 * Copyright (C) 2015  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */

#include "SlideForegroundDecorator.h"

#include <QPainter>

using WAF::SlideForegroundDecorator;


SlideForegroundDecorator::SlideForegroundDecorator(QWidget* _parent) :
	QWidget(_parent)
{
}

void SlideForegroundDecorator::grabParent(const QSize& _size)
{
	resize(_size);
	m_foreground = QPixmap(_size);
	parentWidget()->render(&m_foreground, QPoint(), QRegion(QRect(QPoint(), _size)));
}

void SlideForegroundDecorator::paintEvent(QPaintEvent* _event)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_foreground);

	QWidget::paintEvent(_event);
}

