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


#ifndef EXPANDDECORATOR_H
#define EXPANDDECORATOR_H

#include <QWidget>


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Класс рисующий выезжание области, с последующим заполнением пространства
	 */
	class ExpandDecorator : public QWidget
	{
		Q_OBJECT

		Q_PROPERTY(QRect expandRect READ expandRect WRITE setExpandRect)

	public:
		explicit ExpandDecorator(QWidget* _parent);

		/**
		 * @brief Радиус рисуемой окружности
		 */
		/** @{ */
		QRect expandRect() const;
		void setExpandRect(QRect _expandRect);
		/** @} */

		/**
		 * @brief Сохранить изображение расширяемой области, для последующего его смещения
		 */
		void grabExpandRect();

		/**
		 * @brief Установить цвет заливки
		 */
		void setFillColor(const QColor& _fillColor);

	protected:
		/**
		 * @brief Переопределяется для прорисовки декорации
		 */
		void paintEvent(QPaintEvent* _event);

	private:
		/**
		 * @brief Точка из которой начинается заполенние
		 */
		QPixmap m_expandRectPixmap;

		/**
		 * @brief Прозрачность изображения виджета для прорисовки
		 */
		QRect m_expandRect;

		/**
		 * @brief Цвет растворения
		 */
		QColor m_fillColor;
	};
}

#endif // EXPANDDECORATOR_H
