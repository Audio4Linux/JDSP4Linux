/*
 * Copyright (C) 2015-2017 Dimka Novikov, to@dimkanovikov.pro
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

#include "SideSlideDecorator.h"

#include <QPainter>

using WAF::SideSlideDecorator;


SideSlideDecorator::SideSlideDecorator(QWidget* _parent) :
    QWidget(_parent)
{
    resize(maximumSize());

    m_timeline.setDuration(260);
    m_timeline.setUpdateInterval(40);
    m_timeline.setEasingCurve(QEasingCurve::OutQuad);
    m_timeline.setStartFrame(0);
    m_timeline.setEndFrame(10000);

    m_decorationColor = QColor(0, 0, 0, 0);

    //
    // Анимируем затемнение/осветление
    //
    connect(&m_timeline, &QTimeLine::frameChanged, [=](int _value){
        m_decorationColor = QColor(0, 0, 0, _value/100);
        update();
    });
}

void SideSlideDecorator::grabSlideWidget(QWidget* _slideWidget)
{
    m_slideWidgetPixmap = _slideWidget->grab();
}

void SideSlideDecorator::grabParent()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    //
    // В андройде Qt не умеет рисовать прозрачные виджеты https://bugreports.qt.io/browse/QTBUG-43635
    // поэтому сохранем картинку с изображением подложки
    //
    m_backgroundPixmap = parentWidget()->grab();
#endif
}

void SideSlideDecorator::decorate(bool _dark)
{
    if (m_timeline.state() == QTimeLine::Running) {
        m_timeline.stop();
    }

    m_timeline.setDirection(_dark ? QTimeLine::Forward : QTimeLine::Backward);
    m_timeline.start();
}

QPoint SideSlideDecorator::slidePos() const
{
    return m_slidePos;
}

void SideSlideDecorator::setSlidePos(const QPoint& _pos)
{
    if (m_slidePos != _pos) {
        m_slidePos = _pos;
        update();
    }
}

void SideSlideDecorator::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_backgroundPixmap);
    painter.fillRect(rect(), m_decorationColor);
    painter.drawPixmap(m_slidePos, m_slideWidgetPixmap);

    QWidget::paintEvent(_event);
}

void SideSlideDecorator::mousePressEvent(QMouseEvent *_event)
{
    emit clicked();

    QWidget::mousePressEvent(_event);
}

