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

#ifndef ANIMATIONPRIVATE
#define ANIMATIONPRIVATE

#include <QMap>

class QWidget;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	class AbstractAnimator;

	/**
	 * @brief Данные фасада анимаций
	 */
	class AnimationPrivate
	{
	public:
		/**
		 * @brief Виды аниматоров
		 */
		enum AnimatorType {
			SideSlide,
			Slide,
			Popup,
			CircleFill,
			Expand
		};

	public:
		/**
		 * @brief Есть ли аниматор для заданного виджета
		 */
		bool hasAnimator(QWidget* _widget, AnimatorType _animatorType) const {
			bool contains = false;
			if (m_animators.contains(_animatorType)) {
				contains = m_animators.value(_animatorType).contains(_widget);
			}
			return contains;
		}

		/**
		 * @brief Получить аниматор для заданного виджета
		 */
		AbstractAnimator* animator(QWidget* _widget, AnimatorType _animatorType) const {
			AbstractAnimator* animator = 0;
			if (m_animators.contains(_animatorType)) {
				animator = m_animators.value(_animatorType).value(_widget, 0);
			}
			return animator;
		}

		/**
		 * @brief Сохранить аниматор для заданного виджета
		 */
		void saveAnimator(QWidget* _widget, AbstractAnimator* _animator, AnimatorType _animatorType) {
			if (!hasAnimator(_widget, _animatorType)) {
				QMap<QWidget*, AbstractAnimator*> animators = m_animators.value(_animatorType);
				animators.insert(_widget, _animator);
				m_animators.insert(_animatorType, animators);
			}
		}

	private:
		/**
		 * @brief Карта аниматоров
		 */
		QMap<AnimatorType, QMap<QWidget*, AbstractAnimator*> > m_animators;
	};
}

#endif // ANIMATIONPRIVATE

