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

#ifndef EXPANDANIMATOR_H
#define EXPANDANIMATOR_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

#include <QRect>

class QPropertyAnimation;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class ExpandDecorator;

    /**
     * @brief Аниматор выводящий элемент на передний план и заполняющий пространство
     */
    class ExpandAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit ExpandAnimator(QWidget* _widgetForFill);

        /**
         * @brief Установить область для выведения на передний план
         */
        void setExpandRect(const QRect& _rect);

        /**
         * @brief Установить цвет заливки
         */
        void setFillColor(const QColor& _color);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Заполнить виджет
         */
        /** @{ */
        void animateForward();
        void expandIn();
        /** @} */

        /**
         * @brief Свернуть цветовой круг - очистить виджет
         */
        /** @{ */
        void animateBackward();
        void expandOut();
        /** @} */

    protected:
        /**
         * @brief Переопределяется, чтобы корректировать размер перекрывающего виджета
         */
        bool eventFilter(QObject* _object, QEvent* _event);

    private:
        /**
         * @brief Получить виджет, который нужно заполнить
         */
        QWidget* widgetForFill() const;

    private:
        /**
         * @brief Исходная область для выдвижения
         */
        QRect m_expandRect;

        /**
         * @brief Декоратор, рисующий заполнение
         */
        ExpandDecorator* m_decorator;

        /**
         * @brief Объект для анимирования декоратора
         */
        QPropertyAnimation* m_animation;
    };
}

#endif // EXPANDANIMATOR_H
