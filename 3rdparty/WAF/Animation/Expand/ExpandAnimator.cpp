#include "ExpandAnimator.h"
#include "ExpandDecorator.h"

#include <QEvent>
#include <QPropertyAnimation>

using WAF::ExpandAnimator;
using WAF::ExpandDecorator;


ExpandAnimator::ExpandAnimator(QWidget* _widgetForFill) :
	AbstractAnimator(_widgetForFill),
	m_decorator(new ExpandDecorator(_widgetForFill)),
	m_animation(new QPropertyAnimation(m_decorator, "expandRect"))
{
	Q_ASSERT(_widgetForFill);

	_widgetForFill->installEventFilter(this);

	m_animation->setDuration(400);

	m_decorator->hide();

	connect(m_animation, &QPropertyAnimation::finished, [=] {
		setAnimatedStopped();
		if (isAnimatedBackward()) {
			m_decorator->hide();
		}
	});
}

void ExpandAnimator::setExpandRect(const QRect& _rect)
{
	m_expandRect = _rect;
	m_decorator->setExpandRect(_rect);
}

void ExpandAnimator::setFillColor(const QColor& _color)
{
	m_decorator->setFillColor(_color);
}

int ExpandAnimator::animationDuration() const
{
	return m_animation->duration();
}

void ExpandAnimator::animateForward()
{
	expandIn();
}

void ExpandAnimator::expandIn()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedForward()) return;
	setAnimatedForward();

	//
	// Определим стартовые и финальные позиции для декораций
	//
	QRect startExpandRect = m_expandRect;
	QRect finalExpandRect = widgetForFill()->rect();

	//
	// Позиционируем декораторы
	//
	m_decorator->resize(widgetForFill()->size());
	m_decorator->move(0, 0);
	m_decorator->grabExpandRect();
	m_decorator->show();
	m_decorator->raise();

	//
	// Анимируем виджет
	//
	if (m_animation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_animation->pause();
		m_animation->setDirection(QPropertyAnimation::Backward);
		m_animation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_animation->setEasingCurve(QEasingCurve::InOutQuart);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(startExpandRect);
		m_animation->setEndValue(finalExpandRect);
		m_animation->start();
	}
}

void ExpandAnimator::animateBackward()
{
	expandOut();
}

void ExpandAnimator::expandOut()
{
	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedBackward()) return;
	setAnimatedBackward();

	//
	// Определим стартовые и финальные позиции для декораций
	//
	QRect startExpandRect = widgetForFill()->rect();
	QRect finalExpandRect = m_expandRect;

	//
	// Позиционируем декораторы
	//
	m_decorator->resize(widgetForFill()->size());
	m_decorator->move(0, 0);
	m_decorator->hide();
	m_decorator->grabExpandRect();
	m_decorator->show();
	m_decorator->raise();

	//
	// Анимируем виджет
	//
	if (m_animation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_animation->pause();
		m_animation->setDirection(QPropertyAnimation::Backward);
		m_animation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_animation->setEasingCurve(QEasingCurve::InOutQuart);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(startExpandRect);
		m_animation->setEndValue(finalExpandRect);
		m_animation->start();
	}
}

bool ExpandAnimator::eventFilter(QObject* _object, QEvent* _event)
{
	if (_object == widgetForFill()
		&& _event->type() == QEvent::Resize
		&& m_decorator->isVisible()) {
		m_decorator->resize(widgetForFill()->size());
		m_animation->setEndValue(widgetForFill()->rect());
	}

	return QObject::eventFilter(_object, _event);
}

QWidget* ExpandAnimator::widgetForFill() const
{
	return qobject_cast<QWidget*>(parent());
}
