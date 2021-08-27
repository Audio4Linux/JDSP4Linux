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

#include "StackedWidgetSlideOverAnimator.h"
#include "StackedWidgetSlideOverDecorator.h"

#include <QEvent>
#include <QPropertyAnimation>
#include <QStackedWidget>

using WAF::StackedWidgetSlideOverAnimator;
using WAF::StackedWidgetSlideOverDecorator;


StackedWidgetSlideOverAnimator::StackedWidgetSlideOverAnimator(QStackedWidget* _container, QWidget* _widgetForSlide) :
	AbstractAnimator(_container),
	m_direction(WAF::FromLeftToRight),
	m_coveredWidget(_container->currentWidget()),
	m_decorator(new StackedWidgetSlideOverDecorator(_container, _widgetForSlide)),
	m_animation(new QPropertyAnimation(m_decorator, "pos"))
{
	Q_ASSERT(_container);
	Q_ASSERT(_widgetForSlide);

	_container->installEventFilter(this);

	m_animation->setDuration(400);

	m_decorator->hide();

	connect(m_animation, &QPropertyAnimation::finished, [=] {
		setAnimatedStopped();

		if (isAnimatedForward()) {
			_container->setCurrentWidget(_widgetForSlide);
		}
		m_decorator->hide();
	});
}

void StackedWidgetSlideOverAnimator::updateCoveredWidget()
{
	m_coveredWidget = stackedWidget()->currentWidget();
}

void StackedWidgetSlideOverAnimator::setAnimationDirection(WAF::AnimationDirection _direction)
{
	if (m_direction != _direction) {
		m_direction = _direction;
	}
}

int StackedWidgetSlideOverAnimator::animationDuration() const
{
	return m_animation->duration();
}

void StackedWidgetSlideOverAnimator::animateForward()
{
	slideOverIn();
}

void StackedWidgetSlideOverAnimator::animateStop()
{
    if(m_animation != nullptr)
        m_animation->stop();
}

void StackedWidgetSlideOverAnimator::slideOverIn()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedForward()) return;
	setAnimatedForward();

	//
	// Обновляем изображение виджета в декораторе
	//
	m_decorator->grabWidget();

	//
	// Определим стартовую и финальную позиции для декорации
	//
	QPoint startPos;
	QPoint finalPos;
	switch (m_direction) {
		default:
		case WAF::FromLeftToRight: {
			startPos.setX(-1 * stackedWidget()->width());
			break;
		}

		case WAF::FromRightToLeft: {
			startPos.setX(stackedWidget()->width());
			break;
		}

		case WAF::FromTopToBottom: {
			startPos.setY(-1 * stackedWidget()->height());
			break;
		}

		case WAF::FromBottomToTop: {
			startPos.setY(stackedWidget()->height());
			break;
		}
	}

	//
	// Позиционируем декоратор
	//
	m_decorator->show();
	m_decorator->raise();

	//
	// Выкатываем виджет
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
		m_animation->setEasingCurve(QEasingCurve::InOutExpo);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(startPos);
		m_animation->setEndValue(finalPos);

		m_animation->start();
	}
}

void StackedWidgetSlideOverAnimator::animateBackward()
{
	slideOverOut();
}

void StackedWidgetSlideOverAnimator::slideOverOut()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedBackward()) return;
	setAnimatedBackward();

	//
	// Обновляем изображение виджета в декораторе
	//
	m_decorator->grabWidget();

	//
	// Определим стартовую и финальную позиции для декорации
	//
	QPoint startPos;
	QPoint finalPos;
	switch (m_direction) {
		default:
		case WAF::FromLeftToRight: {
			finalPos.setX(-1 * stackedWidget()->width());
			break;
		}

		case WAF::FromRightToLeft: {
			finalPos.setX(stackedWidget()->width());
			break;
		}

		case WAF::FromTopToBottom: {
			finalPos.setY(-1 * stackedWidget()->height());
			break;
		}

		case WAF::FromBottomToTop: {
			finalPos.setY(stackedWidget()->height());
			break;
		}
	}

	//
	// Делаем виджет, над которым мы выкатывали текущий, активным
	//
	if (QStackedWidget* container = qobject_cast<QStackedWidget*>(stackedWidget())) {
		container->setCurrentWidget(m_coveredWidget);
	}

	//
	// Позиционируем декоратор
	//
	m_decorator->show();
	m_decorator->raise();

	//
	// Закатываем виджет
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
		m_animation->setEasingCurve(QEasingCurve::InOutExpo);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(startPos);
		m_animation->setEndValue(finalPos);

		m_animation->start();
	}
}

bool StackedWidgetSlideOverAnimator::eventFilter(QObject* _object, QEvent* _event)
{
	if (_object == stackedWidget()
		&& _event->type() == QEvent::Resize
		&& m_decorator->isVisible()) {
		m_decorator->grabWidget();
	}

	return QObject::eventFilter(_object, _event);
}

QStackedWidget* StackedWidgetSlideOverAnimator::stackedWidget() const
{
	return qobject_cast<QStackedWidget*>(parent());
}
