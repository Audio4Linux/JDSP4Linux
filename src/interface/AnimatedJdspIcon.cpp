#include "AnimatedJdspIcon.h"

AnimatedJDSPIcon::AnimatedJDSPIcon(QWidget *parent)
	: QWidget(parent)
{
	svgItem->setGeometry(0, 0, 64, 64);
	svgItemU->setGeometry(0, 0, 64, 64);
	svgItemL->setGeometry(0, 0, 64, 64);
	this->setGeometry(0, 0, 64, 64);
	this->setFixedSize(64, 64);

	svgItemU->hide();
	svgItemL->hide();

	group = new QParallelAnimationGroup(this);
	QPropertyAnimation *upper = new QPropertyAnimation(svgItemU, "geometry");
	upper->setDuration(700);
	upper->setEasingCurve(QEasingCurve::Type::OutCirc);
	upper->setStartValue(QRect(0, -64, 64, 64));
	upper->setEndValue(QRect(0, 0, 64, 64));
	group->addAnimation(upper);
	QPropertyAnimation *lower = new QPropertyAnimation(svgItemL, "geometry");
	lower->setDuration(700);
	lower->setEasingCurve(QEasingCurve::Type::OutCirc);
	lower->setStartValue(QRect(0, 128, 64, 64));
	lower->setEndValue(QRect(0, 0, 64, 64));
	group->addAnimation(lower);
}

AnimatedJDSPIcon::~AnimatedJDSPIcon()
{
	delete svgItem;
	delete svgItemU;
	delete svgItemL;
}

void AnimatedJDSPIcon::startAnimation()
{
	if (group->state() == QAbstractAnimation::Running)
	{
		group->stop();
	}

	group->start();
	svgItemU->show();
	svgItemL->show();
}