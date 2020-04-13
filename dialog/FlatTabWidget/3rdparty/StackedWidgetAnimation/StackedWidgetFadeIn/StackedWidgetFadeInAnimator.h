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

#ifndef STACKEDWIDGETFADEINANIMATOR_H
#define STACKEDWIDGETFADEINANIMATOR_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

class QPropertyAnimation;
class QStackedWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class StackedWidgetFadeInDecorator;


    /**
     * @brief Аниматор появления виджета из стека с пропаданием текущего виджета
     */
    class StackedWidgetFadeInAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit StackedWidgetFadeInAnimator(QStackedWidget* _container, QWidget* _fadeWidget);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Отобразить виджет
         */
        /** @{ */
        void animateForward();
        void fadeIn();
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
        QWidget* fadeWidget() const;

    private:
        /**
         * @brief Помошник
         */
        StackedWidgetFadeInDecorator* m_decorator;

        /**
         * @brief Объект для анимирования появления
         */
        QPropertyAnimation* m_animation;
    };
}

#endif // STACKEDWIDGETFADEINANIMATOR_H
