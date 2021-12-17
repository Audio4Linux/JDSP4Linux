#include "FadingLabel.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

FadingLabel::FadingLabel(QWidget* parent) : QLabel(parent)
{
    effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(0.0);
    this->setGraphicsEffect(effect);

    fadeInOut = new QSequentialAnimationGroup(this);
    fadeIn = new QPropertyAnimation(effect, "opacity");
    fadeIn->setDuration(150);
    fadeIn->setEasingCurve(QEasingCurve::Type::OutCurve);
    fadeIn->setStartValue(0);
    fadeIn->setEndValue(1.0);
    fadeOut = new QPropertyAnimation(effect, "opacity");
    fadeOut->setDuration(150);
    fadeOut->setEasingCurve(QEasingCurve::Type::OutCurve);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    fadeInOut->addAnimation(fadeIn);
    fadeInOut->addPause(3000);
    fadeInOut->addAnimation(fadeOut);
}

void FadingLabel::setAnimatedText(const QString &msg, bool highPriority)
{
    if(fadeInOut->state() == QAbstractAnimation::Running)
    {
        if(lastAnimationHighPriority && !highPriority)
        {
            return;
        }

        fadeInOut->stop();
        effect->setOpacity(1);

        fadeIn->setStartValue(1.0);
        fadeIn->setDuration(0);
    }
    else
    {
        fadeIn->setStartValue(0.0);
        fadeIn->setDuration(150);
    }

    this->setText(msg);
    fadeInOut->setCurrentTime(0);
    fadeInOut->start();

    lastAnimationHighPriority = highPriority;
}
