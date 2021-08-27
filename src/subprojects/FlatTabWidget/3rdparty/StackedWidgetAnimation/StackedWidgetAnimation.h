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

#ifndef STACKEDWIDGETANIMATION_H
#define STACKEDWIDGETANIMATION_H

#include "../WAF.h"
#include "../AbstractAnimator.h"

class QColor;
class QRect;
class QStackedWidget;
class QWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    /**
     * @brief Данные фасада
     */
    class StackedWidgetAnimationPrivate;

    /**
     * @brief Фасад доступа к анимациям
     */
    class StackedWidgetAnimation
    {
    public:
        /**
         * @brief Выкатить заданный виджет, смещая текущий активный
         */
        static void slide(QStackedWidget* _container, QWidget* _widget, AnimationDirection _direction);

        /****/

        /**
         * @brief Выкатить заданный виджет поверх текущего
         */
        static void slideOverIn(QStackedWidget* _container, QWidget* _widget, AnimationDirection _direction);

        /**
         * @brief Закатить заданный виджет поверх текущего
         */
        static void slideOverOut(QStackedWidget* _container, QWidget* _widget, AnimationDirection _direction);

        /**
         * @brief Выкатить или закатить заданный виджет поверх текущего
         */
        static void slideOver(QStackedWidget* _container, QWidget* _widget, AnimationDirection _direction, bool _in);

        /****/

        /**
         * @brief Текущий виджет растворяется, а заданный виджет появляется
         */
        static AbstractAnimator* fadeIn(QStackedWidget* _container, QWidget* _widget);

    private:
        /**
         * @brief Данные
         */
        /** @{ */
        static StackedWidgetAnimationPrivate* m_pimpl;
        static StackedWidgetAnimationPrivate* pimpl();
        /** @} */
    };
}

#endif // STACKEDWIDGETANIMATION_H
