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

#ifndef ABSTRACTANIMATOR
#define ABSTRACTANIMATOR

#include <QObject>

/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Абстрактный класс аниматора
	 */
	class AbstractAnimator : public QObject
	{
		Q_OBJECT

	public:
		explicit AbstractAnimator(QObject* _parent = 0) : QObject(_parent) {}

		/**
		 * @brief Длительность анимации
		 */
		virtual int animationDuration() const = 0;

		/**
		 * @brief Выполнить прямую анимацию
		 */
		virtual void animateForward() = 0;

		/**
		 * @brief Выполнить обратную анимацию
		 */
		virtual void animateBackward() = 0;

	protected:
		/**
		 * @brief Установить флаг выполнения анимации
		 */
		/** @{ */
		void setAnimatedForward() {
			m_isAnimated = true;
			m_isAnimatedForward = true;
		}
		void setAnimatedBackward() {
			m_isAnimated = true;
			m_isAnimatedForward = false;
		}
		void setAnimatedStopped() {
			m_isAnimated = false;
		}
		/** @} */

		/**
		 * @brief Выполняется ли анимация в данный момент
		 */
		bool isAnimated() const {
			return m_isAnimated;
		}

		/**
		 * @brief Направление последней анимации
		 */
		/** @{ */
		bool isAnimatedForward() const {
			return m_isAnimatedForward;
		}
		bool isAnimatedBackward() const {
			return !isAnimatedForward();
		}
		/** @} */

	private:
		/**
		 * @brief Выполняется ли анимация в данный момент
		 */
		bool m_isAnimated = false;

		/**
		 * @brief Направление последней анимации
		 */
		bool m_isAnimatedForward = true;
	};
}

#endif // ABSTRACTANIMATOR

