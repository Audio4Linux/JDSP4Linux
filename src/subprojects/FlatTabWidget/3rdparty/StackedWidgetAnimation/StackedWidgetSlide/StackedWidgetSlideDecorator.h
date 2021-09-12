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

#ifndef STACKEDWIDGETSLIDEDECORATOR_H
#define STACKEDWIDGETSLIDEDECORATOR_H

#include <QWidget>

class QStackedWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Класс рисующий выкатываемый виджет
	 */
	class StackedWidgetSlideDecorator : public QWidget
	{
		Q_OBJECT

	public:
		explicit StackedWidgetSlideDecorator(QWidget* _parent, QWidget* _widgetForGrab);

		/**
		 * @brief Сохранить изображение стэка для прорисовки
		 */
		void grabContainer();

		/**
		 * @brief Сохранить изображение виджета для прорисовки
		 */
		void grabWidget();

	protected:
		/**
		 * @brief Переопределяется для прорисовки декорации
		 */
		void paintEvent(QPaintEvent* _event);

	private:
		/**
		 * @brief Виджет, который будем рисовать
		 */
		QWidget* m_widgetForGrab;

		/**
		 * @brief Фоновое изображение
		 */
		QPixmap m_foreground;
	};
}

#endif // STACKEDWIDGETSLIDEDECORATOR_H
