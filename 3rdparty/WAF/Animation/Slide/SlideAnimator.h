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

#ifndef SLIDEANIMATOR_H
#define SLIDEANIMATOR_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

#include <QSize>

class QPropertyAnimation;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class SlideForegroundDecorator;


    /**
     * @brief Аниматор выдвижения виджета
     */
    class SlideAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit SlideAnimator(QWidget* _widgetForSlide);

        /**
         * @brief Установить направление выдвижения
         */
        void setAnimationDirection(AnimationDirection _direction);

        /**
         * @brief Фиксировать фон при анимации (по умолчанию фон фиксируется)
         */
        void setFixBackground(bool _fix);

        /**
         * @brief Запоминать стартовый размер виджета (по-умолчанию не запоминается)
         */
        void setFixStartSize(bool _fix);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Выдвинуть виджет
         */
        /** @{ */
        void animateForward();
        void slideIn();
        /** @} */

        /**
         * @brief Задвинуть виджет
         */
        /** @{ */
        void animateBackward();
        void slideOut();
        /** @} */

    protected:
        /**
         * @brief Переопределяется, чтобы корректировать позицию перекрывающего виджета
         */
        bool eventFilter(QObject* _object, QEvent* _event);

    private:
        /**
         * @brief Зависит ли от анимации ширина (true) или высота (false) анимируемого виджета
         */
        bool isWidth() const;

        /**
         * @brief Зафиксировать размер в направлении изменяемой анимацией стороны
         */
        /** @{ */
        void fixSize(const QSize& _sourceSize, QSize& _targetSize) const;
        void fixSizeOfWidgetForSlide(const QSize& _sourceSize) const;
        /** @} */

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
         * @brief Фиксировать фон при анимации
         */
        bool m_isFixBackground;

        /**
         * @brief Необходимо ли запоминать стартовый размер виджета
         */
        bool m_isFixStartSize;

        /**
         * @brief Исходные метрики анимируемого виджета
         */
        QSize m_startMinSize, m_startMaxSize, m_startSize;

        /**
         * @brief Объект для анимирования выезжания
         */
        QPropertyAnimation* m_animation;

        /**
         * @brief Помошник перекрывающий анимируемый виджет
         */
        SlideForegroundDecorator* m_decorator;
    };
}

#endif // SLIDEANIMATOR_H
