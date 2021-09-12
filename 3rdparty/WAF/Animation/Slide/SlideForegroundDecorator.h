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

#ifndef SLIDEBACKGROUNDDECORATOR_H
#define SLIDEBACKGROUNDDECORATOR_H

#include <QTimeLine>
#include <QWidget>


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Класс перекрывающий передний план выкатываемого виджета
	 * @note Делается это для того, чтобы скрыть деформации компоновщика, при изменении размера виджета
	 */
	class SlideForegroundDecorator : public QWidget
	{
		Q_OBJECT

	public:
		explicit SlideForegroundDecorator(QWidget* _parent);

		/**
		 * @brief Сохранить изображение родительского виджета
		 */
		void grabParent(const QSize& _size);

	protected:
		/**
		 * @brief Переопределяется для прорисовки декорации
		 */
		void paintEvent(QPaintEvent* _event);

	private:
		/**
		 * @brief Фоновое изображение
		 */
		QPixmap m_foreground;
	};
}

#endif // SLIDEBACKGROUNDDECORATOR_H
