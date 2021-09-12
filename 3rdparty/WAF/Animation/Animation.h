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

#ifndef ANIMATION_H
#define ANIMATION_H

#include "../WAF.h"

class QColor;
class QPoint;
class QRect;
class QWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class AbstractAnimator;
    class AnimationPrivate;

    /**
     * @brief Фасад доступа к анимациям
     */
    class Animation
    {
    public:
        /**
         * @brief Выкатить виджет из-за стороны приложения
         */
        static int sideSlideIn(QWidget* _widget, ApplicationSide _side = LeftSide, bool _decorateBackground = true);

        /**
         * @brief Закатить виджет из-за стороны приложения
         */
        static int sideSlideOut(QWidget* _widget, ApplicationSide _side = LeftSide, bool _decorateBackground = true);

        /**
         * @brief Выкатить/закатить виджет из-за стороны приложения
         */
        static int sideSlide(QWidget* _widget, ApplicationSide _side = LeftSide, bool _decorateBackground = true, bool _in = true);

        /****/

        /**
         * @brief Выкатить виджет
         */
        static int slideIn(QWidget* _widget, AnimationDirection _direction, bool _fixBackground = true, bool _fixStartSize = false);

        /**
         * @brief Закатить виджет
         */
        static int slideOut(QWidget* _widget, AnimationDirection _direction, bool _fixBackground = true, bool _fixStartSize = false);

        /**
         * @brief Выкатить/закатить виджет
         */
        static int slide(QWidget* _widget, AnimationDirection _direction, bool _fixBackground = true, bool _fixStartSize = false, bool _in = true);

        /****/

        /**
         * @brief Заполнить/очистить цветовым кругом
         */
        static int circleFillIn(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish = true);

        /**
         * @brief Заполнить/очистить цветовым кругом
         */
        static int circleFillOut(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish = true);

        /**
         * @brief Заполнить/очистить цветовым кругом
         */
        static int circleFill(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish = true, bool _in = true);

        /****/

        /**
         * @brief Вытолкнуть заданную область наверх, затемнив остальную часть экрана
         */
        static int expand(QWidget* _widget, const QRect& _expandRect, const QColor& _fillColor, bool _in = true);

    private:
        /**
         * @brief Запустить заданную анимацию в заданном направлении
         */
        static int runAnimation(AbstractAnimator* _animator, bool _in);

    private:
        /**
         * @brief Данные
         */
        /** @{ */
        static AnimationPrivate* m_pimpl;
        static AnimationPrivate* pimpl();
        /** @} */
    };
}

#endif // ANIMATION_H
