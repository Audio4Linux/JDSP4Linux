#ifndef WAF_H
#define WAF_H


/**
 * Widgets Animation Framework
 */
namespace WAF
{
	/**
	 * @brief Края приложения
	 */
	enum ApplicationSide {
		LeftSide,
		TopSide,
		RightSide,
		BottomSide
	};

	/**
	 * @brief Направление анимации
	 */
	enum AnimationDirection {
		Undefined,
		FromLeftToRight,
		FromTopToBottom,
		FromRightToLeft,
		FromBottomToTop
	};
}

#endif // WAF_H
