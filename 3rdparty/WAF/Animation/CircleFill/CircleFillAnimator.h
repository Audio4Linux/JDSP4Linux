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

#ifndef CIRCLEFILLANIMATOR_H
#define CIRCLEFILLANIMATOR_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

class QPropertyAnimation;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class CircleFillDecorator;

    /**
     * @brief Аниматор заполнения цветным кругом
     */
    class CircleFillAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit CircleFillAnimator(QWidget* _widgetForFill);

        /**
         * @brief Установить точку начала анимации
         */
        void setStartPoint(const QPoint& _point);

        /**
         * @brief Установить цвет заливки
         */
        void setFillColor(const QColor& _color);

        /**
         * @brief Скрывать ли декоратор после окончания анимации
         */
        void setHideAfterFinish(bool _hide);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Заполнить виджет
         */
        /** @{ */
        void animateForward();
        void fillIn();
        /** @} */

        /**
         * @brief Свернуть цветовой круг - очистить виджет
         */
        /** @{ */
        void animateBackward();
        void fillOut();
        /** @} */

        /**
         * @brief Скрыть декоратор
         */
        void hideDecorator();

    private:
        /**
         * @brief Получить виджет, который нужно заполнить
         */
        QWidget* widgetForFill() const;

    private:
        /**
         * @brief Декоратор, рисующий заполнение
         */
        CircleFillDecorator* m_decorator;

        /**
         * @brief Объект для анимирования декоратора
         */
        QPropertyAnimation* m_animation;

        /**
         * @brief Скрывать ли декоратор после завершения анимации
         */
        bool m_hideAfterFinish = true;
    };
}

#endif // CIRCLEFILLANIMATOR_H
