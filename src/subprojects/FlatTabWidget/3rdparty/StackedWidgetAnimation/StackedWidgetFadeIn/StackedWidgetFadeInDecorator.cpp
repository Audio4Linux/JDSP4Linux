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

#include "StackedWidgetFadeInDecorator.h"

#include <QPainter>
#include <QStackedWidget>
#include <QApplication>

using WAF::StackedWidgetFadeInDecorator;


StackedWidgetFadeInDecorator::StackedWidgetFadeInDecorator(QWidget* _parent, QWidget* _fadeWidget) :
	QWidget(_parent),
	m_opacity(1),
	m_fadeWidget(_fadeWidget)
{
	grabFadeWidget();
}

qreal StackedWidgetFadeInDecorator::opacity() const
{
	return m_opacity;
}

void StackedWidgetFadeInDecorator::setOpacity(qreal _opacity)
{
	if (m_opacity != _opacity) {
		m_opacity = _opacity;

		update();
	}
}

void StackedWidgetFadeInDecorator::grabContainer()
{
	if (QStackedWidget* container = qobject_cast<QStackedWidget*>(parentWidget())) {
        container->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7,QWidget#tabHostPage8,QWidget#tabHostPage9{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
        m_containerPixmap = grabWidget(container->currentWidget());
	}
}

void StackedWidgetFadeInDecorator::grabFadeWidget()
{
	m_fadeWidgetPixmap = grabWidget(m_fadeWidget);
}

void StackedWidgetFadeInDecorator::paintEvent(QPaintEvent* _event)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_containerPixmap);
	painter.setOpacity(m_opacity);
	painter.drawPixmap(0, 0, m_fadeWidgetPixmap);

	QWidget::paintEvent(_event);
}

QPixmap StackedWidgetFadeInDecorator::grabWidget(QWidget* _widgetForGrab)
{
	const QSize size = parentWidget()->size();
	_widgetForGrab->resize(size);
	resize(size);
	QPixmap widgetPixmap(size);
    _widgetForGrab->render(&widgetPixmap, QPoint(), QRegion(QRect(QPoint(), size)));
	return widgetPixmap;
}

