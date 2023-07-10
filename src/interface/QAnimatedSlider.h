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
#ifndef QANIMATEDSLIDER_H
#define QANIMATEDSLIDER_H

#include <QPropertyAnimation>
#include <QSlider>

/*!
    \class QAnimatedSlider

    \brief The QAnimatedSlider class provides an extended slider widget with animation capabilities.
 */
class QAnimatedSlider :
	public QSlider
{
	Q_OBJECT

public:
	QAnimatedSlider(QWidget *parent = nullptr);
	~QAnimatedSlider();


	/*!
	    \brief Updates the current stored value and animates this change if \c animate is \c true.
	 */
	void         setValueA(int  value,
	                       bool animate = true);

	/*!
	    \brief Current value of the underlying slider widget
	 */
	int          valueA() const;

    QString      valueString(int overrideValue = -1) const;



	/*!
	    \brief Returns the currently set easing curve used for the animation
	 */
	QEasingCurve easingCurve() const;

	/*!
	    \brief Set a custom easing curve used for the animation.\n
	           By default, the \c QEasingCurve::InOutCirc curve is used.
	 */
	void         setEasingCurve(const QEasingCurve &easingCurve);

	/*!
	    \brief Returns the currently set animation duration
	 */
	int          duration() const;

	/*!
	    \brief Set a custom animation duration.\n
	           By default, this value is set to 300ms.
	 */
	void         setDuration(int duration);

    const QStringList &getCustomValueStrings() const;

    void setCustomValueStrings(const QStringList &newCustomValueStrings);

protected:
    void         onSliderAction(int action);

signals:
	void         valueChangedA(int value);
    void         stringChanged(const QString& str);

private slots:
    void         onTooltipInvalidated();

private:
	QVariantAnimation *anim;
	int cValue                = 0;
	int mDuration             = 300;
	QEasingCurve mEasingCurve = QEasingCurve(QEasingCurve::Type::InOutCirc);
    QStringList customValueStrings;

};

#endif // QANIMATEDSLIDER_H
