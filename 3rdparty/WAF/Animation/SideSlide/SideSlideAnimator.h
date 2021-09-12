#ifndef SIDESLIDEANIMATOR2_H
#define SIDESLIDEANIMATOR2_H

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

class QPropertyAnimation;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class SideSlideDecorator;


    /**
     * @brief Аниматор выдвижения виджета из-за стороны приложения
     */
    class SideSlideAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit SideSlideAnimator(QWidget* _widgetForSlide);

        /**
         * @brief Установить сторону, откуда выдвигать виджет
         */
        void setApplicationSide(ApplicationSide _side);

        /**
         * @brief Использовать ли декорирование фона
         */
        void setDecorateBackground(bool _decorate);

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
         * @brief Переопределяется, чтобы корректировать размер выкатываемого виджета
         */
        bool eventFilter(QObject* _object, QEvent* _event);

    private:
        /**
         * @brief Получить виджет, который нужно анимировать
         */
        QWidget* widgetForSlide() const;

    private:
        /**
         * @brief Сторона из-за которой выкатывать виджет
         */
        ApplicationSide m_side;

        /**
         * @brief Необходимо ли декорировать фон
         */
        bool m_decorateBackground;

        /**
         * @brief Помошник затемняющий фон под выезжающим виджетом
         */
        SideSlideDecorator* m_decorator = nullptr;

        /**
         * @brief Объект для анимирования выезжания
         */
        QPropertyAnimation* m_animation = nullptr;
    };
}

#endif // SIDESLIDEANIMATOR2_H
