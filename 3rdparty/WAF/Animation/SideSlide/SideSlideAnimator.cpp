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

#include "SideSlideAnimator.h"
#include "SideSlideDecorator.h"

#include <cstring>
#include <QEvent>
#include <QPropertyAnimation>
#include <QWidget>

using WAF::SideSlideAnimator;
using WAF::SideSlideDecorator;


SideSlideAnimator::SideSlideAnimator(QWidget* _widgetForSlide) :
    AbstractAnimator(_widgetForSlide),
    m_decorateBackground(true),
    m_decorator(new SideSlideDecorator(_widgetForSlide->parentWidget())),
    m_animation(new QPropertyAnimation(m_decorator, "slidePos"))
{
    Q_ASSERT(_widgetForSlide);
    _widgetForSlide->parentWidget()->installEventFilter(this);

    m_animation->setEasingCurve(QEasingCurve::InQuad);
    m_animation->setDuration(260);

    m_decorator->hide();

    connect(m_animation, &QPropertyAnimation::finished, [this] {
        setAnimatedStopped();
        if (isAnimatedForward()) {
            widgetForSlide()->move(m_decorator->slidePos());
        } else {
            m_decorator->hide();
        }
    });

    connect(m_decorator, &SideSlideDecorator::clicked, this, &SideSlideAnimator::slideOut);
}

void SideSlideAnimator::setApplicationSide(WAF::ApplicationSide _side)
{
    if (m_side != _side) {
        m_side = _side;
    }
}

void SideSlideAnimator::setDecorateBackground(bool _decorate)
{
    if (m_decorateBackground != _decorate) {
        m_decorateBackground = _decorate;
    }
}

int SideSlideAnimator::animationDuration() const
{
    return m_animation->duration();
}

void SideSlideAnimator::animateForward()
{
    slideIn();
}
#include <QDebug>
void SideSlideAnimator::slideIn()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedForward()) return;
    setAnimatedForward();

    //
    // Прячем виджет для анимирования
    //
    widgetForSlide()->lower();
    widgetForSlide()->move(-widgetForSlide()->width(), -widgetForSlide()->height());
    widgetForSlide()->show();
    widgetForSlide()->resize(widgetForSlide()->sizeHint());

    //
    // Определим самый верхний виджет
    //
    QWidget* topWidget = widgetForSlide()->parentWidget();
    while (topWidget->parentWidget() != 0 && !topWidget->inherits("QDialog")) {
        topWidget = topWidget->parentWidget();
    }

    //
    // Определим финальный размер и положение выезжающего виджета
    //
    // TODO: скорректировать размер, если конфликтует с размером приложения
    //
    QSize finalSize = widgetForSlide()->size();
    QPoint startPosition, finalPosition;
    switch (m_side) {
        case WAF::LeftSide: {
            finalSize.setHeight(topWidget->height());
            startPosition = QPoint(-finalSize.width(), 0);
            finalPosition = QPoint(0, 0);
            break;
        }

        case WAF::TopSide: {
            finalSize.setWidth(topWidget->width());
            startPosition = QPoint(0, -finalSize.height());
            finalPosition = QPoint(0, 0);
            break;
        }

        case WAF::RightSide: {
            finalSize.setHeight(topWidget->height());
            startPosition = QPoint(topWidget->width(), 0);
            finalPosition = QPoint(topWidget->width() - finalSize.width(), 0);
            break;
        }

        case WAF::BottomSide: {
            finalSize.setWidth(topWidget->width());
            startPosition = QPoint(0, topWidget->height());
            finalPosition = QPoint(0, topWidget->height() - finalSize.height());
            break;
        }
    }

    //
    // Позиционируем декоратор
    //
    if (m_decorateBackground) {
        m_decorator->setParent(topWidget);
        m_decorator->move(0, 0);
        m_decorator->grabParent();
        m_decorator->show();
        m_decorator->raise();
    }

    //
    // Позиционируем виджет для анимации в исходное положение и настраиваем его размер
    //
    widgetForSlide()->move(startPosition);
    widgetForSlide()->resize(finalSize);
    widgetForSlide()->raise();

    //
    // Сохраним изображение выкатываемого виджета в декораторе
    //
    m_decorator->grabSlideWidget(widgetForSlide());

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
        m_animation->setEasingCurve(QEasingCurve::OutQuart);
        m_animation->setDirection(QPropertyAnimation::Forward);
        m_animation->setStartValue(startPosition);
        m_animation->setEndValue(finalPosition);
        m_animation->start();
    }

    //
    // Декорируем фон
    //
    if (m_decorateBackground) {
        const bool DARKER = true;
        m_decorator->decorate(DARKER);
    }
}

void SideSlideAnimator::animateBackward()
{
    slideOut();
}

void SideSlideAnimator::slideOut()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedBackward()) return;
    setAnimatedBackward();

    m_decorator->grabSlideWidget(widgetForSlide());

    if (widgetForSlide()->isVisible()) {
        //
        // Определим самый верхний виджет
        //
        QWidget* topWidget = widgetForSlide()->parentWidget();
        while (topWidget->parentWidget() != 0 && !topWidget->inherits("QDialog")) {
            topWidget = topWidget->parentWidget();
        }

        //
        // Прячем анимируемый виджет
        //
        widgetForSlide()->hide();

        //
        // Определяем позиции прокатывания
        //
        const QSize finalSize = widgetForSlide()->size();
        QPoint startPosition = widgetForSlide()->pos(), finalPosition;
        switch (m_side) {
            case WAF::LeftSide: {
                startPosition = QPoint(0, 0);
                finalPosition = QPoint(-finalSize.width(), 0);
                break;
            }

            case WAF::TopSide: {
                startPosition = QPoint(0, 0);
                finalPosition = QPoint(0, -finalSize.height());
                break;
            }

            case WAF::RightSide: {
                startPosition = QPoint(topWidget->width() - finalSize.width(), 0);
                finalPosition = QPoint(topWidget->width(), 0);
                break;
            }

            case WAF::BottomSide: {
                startPosition = QPoint(0, topWidget->height() - finalSize.height());
                finalPosition = QPoint(0, topWidget->height());
                break;
            }
        }

        //
        // Анимируем закатывание виджета
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
            m_animation->setEasingCurve(QEasingCurve::InQuart);
            m_animation->setDirection(QPropertyAnimation::Forward);
            m_animation->setStartValue(startPosition);
            m_animation->setEndValue(finalPosition);
            m_animation->start();
        }

        //
        // Декорируем фон
        //
        if (m_decorateBackground) {
            const bool LIGHTER = false;
            m_decorator->decorate(LIGHTER);
        }
    }
}

bool SideSlideAnimator::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == widgetForSlide()->parentWidget()
        && _event->type() == QEvent::Resize) {

        QWidget* widgetForSlideParent = widgetForSlide()->parentWidget();
        switch (m_side) {
            case WAF::RightSide: {
                widgetForSlide()->move(widgetForSlideParent->width() - widgetForSlide()->width(), 0);
                //
                // проваливаемся, для корректировки высоты
                //
                [[fallthrough]];
            }
            case WAF::LeftSide: {
                widgetForSlide()->resize(widgetForSlide()->width(), widgetForSlideParent->height());
                break;
            }

            case WAF::BottomSide: {
                widgetForSlide()->move(0, widgetForSlideParent->height() - widgetForSlide()->height());
                //
                // Проваливаемся, для корректировки ширины
                //
                [[fallthrough]];
            }
            case WAF::TopSide: {
                widgetForSlide()->resize(widgetForSlideParent->width(), widgetForSlide()->height());
                break;
            }
        }

        m_decorator->setSlidePos(widgetForSlide()->geometry().topLeft());

        //
        // Сохраним картинку изменённого выкатываемого виджета
        //
        m_decorator->grabSlideWidget(widgetForSlide());

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        //
        // В андройде нужно перерисовать фоновое изображение
        //
        m_decorator->hide();
        m_decorator->grabParent();
        m_decorator->show();
#endif
    }

    return QObject::eventFilter(_object, _event);
}

QWidget* SideSlideAnimator::widgetForSlide() const
{
    return qobject_cast<QWidget*>(parent());
}
