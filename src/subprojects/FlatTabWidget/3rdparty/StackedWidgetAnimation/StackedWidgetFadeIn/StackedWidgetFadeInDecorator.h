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

#ifndef STACKEDWIDGETFADEINDECORATOR_H
#define STACKEDWIDGETFADEINDECORATOR_H

#include <QWidget>

class QStackedWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Класс рисующий растворяемый виджет
	 */
	class StackedWidgetFadeInDecorator : public QWidget
	{
		Q_OBJECT

		Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

	public:
		explicit StackedWidgetFadeInDecorator(QWidget* _parent, QWidget* _fadeWidget);

		/**
		 * @brief Свойство прозрачности изображения виджета для прорисовки
		 */
		/** @{ */
		qreal opacity() const;
		void setOpacity(qreal _opacity);
		/** @} */

		/**
		 * @brief Установить цвет растворения
		 */
		void setFadeInColor(const QColor& _fadeInColor);

		/**
		 * @brief Сохранить изображение стэка для прорисовки
		 */
		void grabContainer();

		/**
		 * @brief Сохранить изображение виджета для прорисовки
		 */
		void grabFadeWidget();

	protected:
		/**
		 * @brief Переопределяется для прорисовки декорации
		 */
		void paintEvent(QPaintEvent* _event);

	private:
		/**
		 * @brief Сохранить изображение заданного виджета
		 */
		QPixmap grabWidget(QWidget* _widgetForGrab);

	private:
		/**
		 * @brief Прозрачность изображения виджета для прорисовки
		 */
		qreal m_opacity;

		/**
		 * @brief Виджет, который будем рисовать
		 */
		QWidget* m_fadeWidget;

		/**
		 * @brief Изображение контейнера
		 */
		QPixmap m_containerPixmap;

		/**
		 * @brief Изображение появляющегося виджета
		 */
		QPixmap m_fadeWidgetPixmap;
	};
}

#endif // STACKEDWIDGETFADEINDECORATOR_H
