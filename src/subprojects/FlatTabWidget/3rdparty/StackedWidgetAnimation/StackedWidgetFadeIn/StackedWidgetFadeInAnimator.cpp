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

#include "StackedWidgetFadeInAnimator.h"
#include "StackedWidgetFadeInDecorator.h"

#include <QEvent>
#include <QPropertyAnimation>
#include <QStackedWidget>

using WAF::StackedWidgetFadeInAnimator;
using WAF::StackedWidgetFadeInDecorator;


StackedWidgetFadeInAnimator::StackedWidgetFadeInAnimator(QStackedWidget* _container, QWidget* _fadeWidget) :
	AbstractAnimator(_container),
	m_decorator(new StackedWidgetFadeInDecorator(_container, _fadeWidget)),
	m_animation(new QPropertyAnimation(m_decorator, "opacity"))
{
	Q_ASSERT(_container);
	Q_ASSERT(_fadeWidget);

	_container->installEventFilter(this);

	m_animation->setDuration(200);

	m_decorator->hide();

	connect(m_animation, &QPropertyAnimation::finished, [=] {
		setAnimatedStopped();

		if (m_animation->direction() == QPropertyAnimation::Forward) {
			_container->setCurrentWidget(_fadeWidget);
		}
		m_decorator->hide();
	});
}

int StackedWidgetFadeInAnimator::animationDuration() const
{
	return m_animation->duration();
}

void StackedWidgetFadeInAnimator::animateForward()
{
	fadeIn();
}

void StackedWidgetFadeInAnimator::fadeIn()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedForward()) return;
	setAnimatedForward();

	//
	// Обновляем изображения виджетов в декораторах
	//
	m_decorator->grabContainer();
	m_decorator->grabFadeWidget();

	//
	// Определим стартовые и финальные позиции для декораций
	//
	qreal startOpacity = 0.;
	qreal finalOpacity = 1.;

	//
	// Позиционируем декораторы
	//
	m_decorator->setOpacity(startOpacity);
	m_decorator->move(0, 0);
	m_decorator->show();
	m_decorator->raise();

	//
	// Анимируем виджет
	//
	if (m_animation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_animation->pause();
		m_animation->setDirection(QPropertyAnimation::Backward);
		m_animation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_animation->setEasingCurve(QEasingCurve::InQuad);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(startOpacity);
		m_animation->setEndValue(finalOpacity);

		m_animation->start();
    }
}

void StackedWidgetFadeInAnimator::animateStop()
{
    if(m_animation != nullptr){
        m_animation->stop();
        m_animation->emit finished();
    }
}

bool StackedWidgetFadeInAnimator::eventFilter(QObject* _object, QEvent* _event)
{
	if (_object == fadeWidget()
		&& _event->type() == QEvent::Resize
		&& m_decorator->isVisible()) {
		m_decorator->grabContainer();
		m_decorator->grabFadeWidget();
	}

	return QObject::eventFilter(_object, _event);
}

QWidget* StackedWidgetFadeInAnimator::fadeWidget() const
{
	return qobject_cast<QWidget*>(parent());
}
