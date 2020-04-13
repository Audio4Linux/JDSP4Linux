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

#include "StackedWidgetSlideOverDecorator.h"

#include <QPainter>
#include <QStackedWidget>

using WAF::StackedWidgetSlideOverDecorator;


StackedWidgetSlideOverDecorator::StackedWidgetSlideOverDecorator(QWidget* _parent, QWidget* _widgetForGrab) :
	QWidget(_parent),
	m_widgetForGrab(_widgetForGrab)
{
	grabWidget();
}

void StackedWidgetSlideOverDecorator::grabContainer()
{
	if (QStackedWidget* container = qobject_cast<QStackedWidget*>(parentWidget())) {
		m_widgetForGrab = container->currentWidget();
		grabWidget();
	}
}

void StackedWidgetSlideOverDecorator::grabWidget()
{
	const QSize size = parentWidget()->size();
	m_widgetForGrab->resize(size);
	resize(size);
	m_foreground = QPixmap(size);
	m_widgetForGrab->render(&m_foreground, QPoint(), QRegion(QRect(QPoint(), size)));
}

void StackedWidgetSlideOverDecorator::paintEvent(QPaintEvent* _event)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_foreground);

	QWidget::paintEvent(_event);
}

