#include "QAnimatedSlider.h"
#include <QKeyEvent>
/*
 *  MIT License

    Copyright (c) 2020 Tim Schneeberger (ThePBone) <tim.schneeberger(at)outlook.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 *
 */

QAnimatedSlider::QAnimatedSlider(QWidget *parent)
	: QSlider(parent)
{
	anim = new QPropertyAnimation(this, "value");
	connect(this, &QSlider::sliderReleased, [this] {
		cValue = value();
		emit valueChangedA(cValue);
	});
	connect(this, &QSlider::sliderMoved, [this](int position) {
		cValue = position;
		emit valueChangedA(cValue);
	});
}

QAnimatedSlider::~QAnimatedSlider()
{
    anim->deleteLater();
}

void QAnimatedSlider::setValueA(int  val,
                                bool animate)
{
	cValue = val;

	if (animate)
	{
		anim->stop();
		anim->setDuration(mDuration);
		anim->setEasingCurve(mEasingCurve);
		anim->setStartValue(QSlider::value());
		anim->setEndValue(val);
		anim->start();
	}
	else
	{
		QSlider::setValue(val);
	}
}

int QAnimatedSlider::valueA() const
{
	return cValue;
}

QEasingCurve QAnimatedSlider::easingCurve() const
{
	return mEasingCurve;
}

void QAnimatedSlider::setEasingCurve(const QEasingCurve &easingCurve)
{
	mEasingCurve = easingCurve;
}

int QAnimatedSlider::duration() const
{
	return mDuration;
}

void QAnimatedSlider::setDuration(int duration)
{
	mDuration = duration;
}

bool QAnimatedSlider::event(QEvent *ev)
{
	ev->ignore();

	if (ev->type() == QEvent::Type::KeyRelease)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);

		switch (keyEvent->key())
		{
			case Qt::Key::Key_Up:
			case Qt::Key::Key_Down:
			case Qt::Key::Key_Left:
			case Qt::Key::Key_Right:
			case Qt::Key::Key_PageUp:
			case Qt::Key::Key_PageDown:
				cValue = value();
				emit valueChangedA(cValue);
				return true;
			default:
				QSlider::event(ev);
		}

	}

	return QSlider::event(ev);
}
