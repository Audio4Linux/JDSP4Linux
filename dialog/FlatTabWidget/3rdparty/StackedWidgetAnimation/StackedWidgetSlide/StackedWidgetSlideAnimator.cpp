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

#include "StackedWidgetSlideAnimator.h"
#include "StackedWidgetSlideDecorator.h"

#include <QEvent>
#include <QPropertyAnimation>
#include <QStackedWidget>

using WAF::StackedWidgetSlideAnimator;
using WAF::StackedWidgetSlideDecorator;


StackedWidgetSlideAnimator::StackedWidgetSlideAnimator(QStackedWidget* _container, QWidget* _widgetForSlide) :
	AbstractAnimator(_container),
	m_direction(WAF::FromLeftToRight),
	m_containerDecorator(new StackedWidgetSlideDecorator(_container, _container->currentWidget())),
	m_widgetDecorator(new StackedWidgetSlideDecorator(_container, _widgetForSlide)),
	m_containerAnimation(new QPropertyAnimation(m_containerDecorator, "pos")),
	m_widgetAnimation(new QPropertyAnimation(m_widgetDecorator, "pos"))
{
	Q_ASSERT(_container);
	Q_ASSERT(_widgetForSlide);

	_container->installEventFilter(this);

	m_containerAnimation->setDuration(400);
	m_widgetAnimation->setDuration(400);

	m_containerDecorator->hide();
	m_widgetDecorator->hide();

	connect(m_widgetAnimation, &QPropertyAnimation::finished, [=] {
		setAnimatedStopped();

		_container->setCurrentWidget(_widgetForSlide);
		m_containerDecorator->hide();
		m_widgetDecorator->hide();
	});
}

void StackedWidgetSlideAnimator::setAnimationDirection(WAF::AnimationDirection _direction)
{
	if (m_direction != _direction) {
		m_direction = _direction;
	}
}

int StackedWidgetSlideAnimator::animationDuration() const
{
	return m_containerAnimation->duration();
}

void StackedWidgetSlideAnimator::animateForward()
{
	slide();
}

void StackedWidgetSlideAnimator::animateStop()
{
    //not implemented
}

void StackedWidgetSlideAnimator::slide()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedForward()) return;
	setAnimatedForward();

	//
	// Обновляем изображения виджетов в декораторах
	//
	m_containerDecorator->grabContainer();
	m_widgetDecorator->grabWidget();

	//
	// Определим стартовые и финальные позиции для декораций
	//
	QPoint containerStartPos;
	QPoint containerFinalPos;
	QPoint widgetStartPos;
	QPoint widgetFinalPos;
	switch (m_direction) {
		default:
		case WAF::FromLeftToRight: {
			containerFinalPos.setX(widgetForSlide()->width());
			widgetStartPos.setX(-1 * widgetForSlide()->width());
			break;
		}

		case WAF::FromRightToLeft: {
			containerFinalPos.setX(-1 * widgetForSlide()->width());
			widgetStartPos.setX(widgetForSlide()->width());
			break;
		}

		case WAF::FromTopToBottom: {
			containerFinalPos.setY(widgetForSlide()->height());
			widgetStartPos.setY(-1 * widgetForSlide()->height());
			break;
		}

		case WAF::FromBottomToTop: {
			containerFinalPos.setY(-1 * widgetForSlide()->height());
			widgetStartPos.setY(widgetForSlide()->height());
			break;
		}
	}

	//
	// Позиционируем декораторы
	//
	m_containerDecorator->move(containerStartPos);
	m_containerDecorator->show();
	m_containerDecorator->raise();
	m_widgetDecorator->move(widgetStartPos);
	m_widgetDecorator->show();
	m_widgetDecorator->raise();

	//
	// Выкатываем виджет
	//
	if (m_widgetAnimation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_containerAnimation->pause();
		m_containerAnimation->setDirection(QPropertyAnimation::Backward);
		m_containerAnimation->resume();
		m_widgetAnimation->pause();
		m_widgetAnimation->setDirection(QPropertyAnimation::Backward);
		m_widgetAnimation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_containerAnimation->setEasingCurve(QEasingCurve::InOutExpo);
		m_containerAnimation->setDirection(QPropertyAnimation::Forward);
		m_containerAnimation->setStartValue(containerStartPos);
		m_containerAnimation->setEndValue(containerFinalPos);
		m_widgetAnimation->setEasingCurve(QEasingCurve::InOutExpo);
		m_widgetAnimation->setDirection(QPropertyAnimation::Forward);
		m_widgetAnimation->setStartValue(widgetStartPos);
		m_widgetAnimation->setEndValue(widgetFinalPos);

		m_containerAnimation->start();
		m_widgetAnimation->start();
	}
}

bool StackedWidgetSlideAnimator::eventFilter(QObject* _object, QEvent* _event)
{
	if (_object == widgetForSlide()
		&& _event->type() == QEvent::Resize
		&& m_containerDecorator->isVisible()
		&& m_widgetDecorator->isVisible()) {
		m_containerDecorator->grabWidget();
		m_widgetDecorator->grabWidget();
	}

	return QObject::eventFilter(_object, _event);
}

QWidget* StackedWidgetSlideAnimator::widgetForSlide() const
{
	return qobject_cast<QWidget*>(parent());
}
