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

#ifndef STACKEDWIDGETSLIDEANIMATOR_H
#define STACKEDWIDGETSLIDEANIMATOR_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

class QPropertyAnimation;
class QStackedWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class StackedWidgetSlideDecorator;


    /**
     * @brief Аниматор выдвижения виджета из стека со смещением текущего виджета
     */
    class StackedWidgetSlideAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit StackedWidgetSlideAnimator(QStackedWidget* _container, QWidget* _widgetForSlide);

        /**
         * @brief Установить направление выдвижения
         */
        void setAnimationDirection(AnimationDirection _direction);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Выдвинуть виджет
         */
        /** @{ */
        void animateForward();
        void slide();
        /** @} */

        /**
         * @brief Обратной анимации для данного случая нет
         */
        void animateBackward() {}
        void animateStop();

    protected:
        /**
         * @brief Переопределяется, чтобы корректировать позицию перекрывающего виджета
         */
        bool eventFilter(QObject* _object, QEvent* _event);

    private:
        /**
         * @brief Получить виджет, который нужно анимировать
         */
        QWidget* widgetForSlide() const;

    private:
        /**
         * @brief Направление, по которому выкатывать виджет
         */
        AnimationDirection m_direction;

        /**
         * @brief Помошники
         */
        /** @{ */
        StackedWidgetSlideDecorator* m_containerDecorator;
        StackedWidgetSlideDecorator* m_widgetDecorator;
        /** @} */

        /**
         * @brief Объекты для анимирования выезжания
         */
        /** @{ */
        QPropertyAnimation* m_containerAnimation;
        QPropertyAnimation* m_widgetAnimation;
        /** @} */
    };
}

#endif // STACKEDWIDGETSLIDEANIMATOR_H
