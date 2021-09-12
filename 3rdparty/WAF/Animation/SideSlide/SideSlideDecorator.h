#ifndef SIDESLIDEDECORATOR_H
#define SIDESLIDEDECORATOR_H

#include <QTimeLine>
#include <QWidget>


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    /**
     * @brief Класс декорирующий анимацию выкатывания
     */
    class SideSlideDecorator : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(QPoint slidePos READ slidePos WRITE setSlidePos)

    public:
        explicit SideSlideDecorator(QWidget* _parent);

        /**
         * @brief Сохранить изображение выкатываемого виджета
         */
        void grabSlideWidget(QWidget* _slideWidget);

        /**
         * @brief Сохранить изображение родительского виджета
         */
        void grabParent();

        /**
         * @brief Задекорировать фон
         */
        void decorate(bool _dark);

        /**
         * @brief Получить позицию выкатываемого виджета
         */
        QPoint slidePos() const;

        /**
         * @brief Установить позицию выкатываемого виджета
         */
        void setSlidePos(const QPoint& _pos);

    signals:
        /**
         * @brief На виджете произведён щелчёк мышью
         */
        void clicked();

    protected:
        /**
         * @brief Переопределяется для прорисовки декорации
         */
        void paintEvent(QPaintEvent* _event);

        /**
         * @brief Переопределяется для отлавливания щелчка мышью
         */
        void mousePressEvent(QMouseEvent* _event);

    private:
        /**
         * @brief Позиция выкатываемого виджета
         */
        QPoint m_slidePos;

        /**
         * @brief Изображение выкатываемого виджета
         */
        QPixmap m_slideWidgetPixmap;

        /**
         * @brief Таймлайн для реализации анимированного декорирования
         */
        QTimeLine m_timeline;

        /**
         * @brief Фоновое изображение
         */
        QPixmap m_backgroundPixmap;

        /**
         * @brief Цвет декорирования фона
         */
        QColor m_decorationColor;
    };
}

#endif // SIDESLIDEDECORATOR_H
