/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ThePBone <tim.schneeberger(at)outlook.de> (c) 2020

    Design inspired by
    https://github.com/vipersaudio/viper4android_fx
 */

#include "JdspImpResToolbox.h"
#include "LiquidEqualizerWidget.h"

#include <QDebug>
#include <QLinearGradient>
#include <QPainterPath>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LiquidEqualizerWidget::LiquidEqualizerWidget(QWidget *parent)
	: QWidget(parent)
{
	for (int i = 0; i < BANDS_NUM; i++)
	{
		anim[i] = new QVariantAnimation(this);
	}

	dispFreq             = new double[RESOLUTION];
	response             = new float[RESOLUTION];
	precomputeCurveXAxis = new float[RESOLUTION];

	for (int i = 0; i < RESOLUTION; i++)
	{
		dispFreq[i] = reverseProjectX(i / (float) (RESOLUTION - 1));
	}

	for (int i = 0; i < RESOLUTION; i++)
	{
		precomputeCurveXAxis[i] = projectX(dispFreq[i]);
	}

	int i = 0;

	for (int freq = MIN_FREQ; freq < MAX_FREQ;)
	{
		precomputeFreqAxis[i] = projectX(freq);

		if (freq < 100)
		{
			freq += 10;
		}
		else if (freq < 1000)
		{
			freq += 100;
		}
		else if (freq < 10000)
		{
			freq += 1000;
		}
		else
		{
			freq += 10000;
		}

		i++;
	}

	connect(this, &LiquidEqualizerWidget::redrawRequired, [this]() {
		repaint();
	});
}

LiquidEqualizerWidget::~LiquidEqualizerWidget()
{}

void LiquidEqualizerWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	float y = (float) event->y() / (float) height();

	if (y < 0 || y > 1)
	{
		return;
	}

	float dB = reverseProjectY(y);
	setBand(getIndexUnderMouse(event->x()), dB, false);
}

int LiquidEqualizerWidget::getAnimationDuration() const
{
	return mAnimationDuration;
}

void LiquidEqualizerWidget::setAnimationDuration(int animationDuration)
{
	mAnimationDuration = animationDuration;
}

QColor LiquidEqualizerWidget::getAccentColor() const
{
	return mAccentColor;
}

void LiquidEqualizerWidget::setAccentColor(const QColor &accentColor)
{
	mAccentColor = accentColor;
	emit redrawRequired();
}

bool LiquidEqualizerWidget::getAlwaysDrawHandles() const
{
	return mAlwaysDrawHandles;
}

void LiquidEqualizerWidget::setAlwaysDrawHandles(bool alwaysDrawHandles)
{
	mAlwaysDrawHandles = alwaysDrawHandles;
	emit redrawRequired();
}

bool LiquidEqualizerWidget::getGridVisible() const
{
	return mGridVisible;
}

void LiquidEqualizerWidget::setGridVisible(bool gridVisible)
{
	mGridVisible = gridVisible;
	emit redrawRequired();
}

void LiquidEqualizerWidget::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event)
	mHoldDown = true;
}

void LiquidEqualizerWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event)
	mHoldDown = false;
	emit bandsUpdated();
	emit mouseReleased();
	repaint();
}

void LiquidEqualizerWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (mHoldDown)
	{
		float y = (float) event->y() / (float) height();

		if (y < 0 || y > 1)
		{
			return;
		}

		float dB = reverseProjectY(y);
		setBand(getIndexUnderMouse(event->x()), dB, false, true);
	}
}

void LiquidEqualizerWidget::keyPressEvent(QKeyEvent *event)
{
	QPoint point      = this->mapFromGlobal(QCursor::pos());
	int    index      = getIndexUnderMouse(point.x());

	QRect  widgetRect = this->geometry();
	widgetRect.moveTopLeft(this->parentWidget()->mapToGlobal(widgetRect.topLeft()));

	if (widgetRect.contains(QCursor::pos()))
	{
		mHoldDown = true;

		if (mKey_LastMousePos != QCursor::pos())
		{
			mKey_CurrentIndex = index;
		}

		if (event->key() == Qt::Key::Key_Up)
		{
			float cur = getBand(mKey_CurrentIndex);
			setBand(mKey_CurrentIndex, cur >= 12.0 ? 12.0 : cur + 0.5, false, true);
			emit  bandsUpdated();
		}
		else if (event->key() == Qt::Key::Key_Down)
		{
			float cur = getBand(mKey_CurrentIndex);
			setBand(mKey_CurrentIndex, cur <= -12.0 ? -12.0 : cur - 0.5, false, true);
			emit  bandsUpdated();
		}
		else if (event->key() == Qt::Key::Key_Left && mKey_LastMousePos == QCursor::pos())
		{
			if (mKey_CurrentIndex > 0)
			{
				mKey_CurrentIndex--;
			}

			setBand(mKey_CurrentIndex, getBand(mKey_CurrentIndex), false, true);
		}
		else if (event->key() == Qt::Key::Key_Right && mKey_LastMousePos == QCursor::pos())
		{
			if (mKey_CurrentIndex < BANDS_NUM)
			{
				mKey_CurrentIndex++;
			}

			setBand(mKey_CurrentIndex, getBand(mKey_CurrentIndex), false, true);
		}
	}

	mKey_LastMousePos = QCursor::pos();
}

void LiquidEqualizerWidget::keyReleaseEvent(QKeyEvent *event)
{
	Q_UNUSED(event)
	QPoint point = this->mapFromGlobal(QCursor::pos());
	int index = getIndexUnderMouse(point.x());

	if (index == mKey_CurrentIndex)
	{
		mHoldDown = false;
		repaint();
	}
}

void LiquidEqualizerWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	mWidth  = this->width() + 1;
	mHeight = this->height() + 1;

	QPainterPath frequencyResponse;
	float        x, y;

    ComputeEqResponse(mFreq.toStdVector().data(), mLevels, 1, RESOLUTION, dispFreq, response);

	for (int i = 0; i < RESOLUTION; i++)
	{
		/* Magnitude response, dB */
		x = precomputeCurveXAxis[i] * mWidth;
		y = projectY(response[i]) * mHeight;

		/* Set starting point at first point */
		if (i == 0)
		{
			frequencyResponse.moveTo(x, y);
		}
		else
		{
			frequencyResponse.lineTo(x, y);
		}
	}

	QPainterPath    frequencyResponseBackground;
	frequencyResponseBackground.addPath(frequencyResponse);
	frequencyResponseBackground.lineTo(mWidth, mHeight);
	frequencyResponseBackground.lineTo(0.0f,   mHeight);

	QLinearGradient gradient(QPoint(width(), 0), QPoint(width(), height()));
	gradient.setColorAt(0.0, mAccentColor);
	gradient.setColorAt(1.0, QColor(0, 0, 0, 0));

	if (mGridVisible)
	{
		mGridLines.begin(this);
		mGridLines.setPen(QPen(palette().mid(), 0.75, Qt::PenStyle::SolidLine, Qt::PenCapStyle::SquareCap));
		mGridLines.setRenderHint(QPainter::RenderHint::Antialiasing, true);

		float decibel = MIN_DB + 3.0f;

		while (decibel <= MAX_DB - 3)
		{
			float y = projectY(decibel) * mHeight;
			mGridLines.drawLine(0.0f, y, mWidth - 1, y);
			decibel += 3;
		}

		mGridLines.end();
	}

	mFrequencyResponseBg.begin(this);
	mFrequencyResponseBg.setBrush(gradient);
	mFrequencyResponseBg.setRenderHint(QPainter::RenderHint::Antialiasing, true);
	mFrequencyResponseBg.drawPath(frequencyResponseBackground);
	mFrequencyResponseBg.end();

	mFrequencyResponseHighlight.begin(this);
	mFrequencyResponseHighlight.setPen(QPen(QBrush(mAccentColor), 3, Qt::PenStyle::SolidLine, Qt::PenCapStyle::SquareCap));
	mFrequencyResponseHighlight.setRenderHint(QPainter::RenderHint::Antialiasing, true);
	mFrequencyResponseHighlight.drawPath(frequencyResponse);

	// Take a font sample from another painter for later
	mFrequencyResponseHighlight.end();

	for (int i = 0; i < BANDS_NUM; i++)
	{
		double  frequency = mFreq[i];
		double  x         = projectX(frequency) * mWidth;
		double  y         = projectY(mLevels[i]) * mHeight;
		QString frequencyText;
		frequencyText.sprintf(frequency < 1000 ? "%.0f" : "%.0fk", frequency < 1000 ? frequency : frequency / 1000);
		QString gainText;
		gainText.sprintf("%.1f", mLevels[i]);

		if ((mManual && i == mSelectedBand && mHoldDown) || mAlwaysDrawHandles)
		{
			mControlBarKnob.begin(this);
			mControlBarKnob.setBrush(mAccentColor);
			mControlBarKnob.setPen(mAccentColor.lighter(50));
			mControlBarKnob.setRenderHint(QPainter::RenderHint::Antialiasing, true);
			mControlBarKnob.drawEllipse(x - 8, y - 9, 18.0f, 18.0f);
			mControlBarKnob.end();
		}

		mControlBarText.begin(this);

		QFont font = mControlBarText.font();
		font.setPointSize(8);
		mControlBarText.setFont(font);
		mControlBarText.drawText(x, 19,              gainText);
		mControlBarText.drawText(x, mHeight - 16.0f, frequencyText);
		mControlBarText.end();
	}
}

void LiquidEqualizerWidget::setBand(int    i,
                                    double value,
                                    bool   animate,
                                    bool   manual)
{
	mSelectedBand    = i;
	mManual          = manual;

	mActualLevels[i] = value;

	if (animate)
	{
		if (anim[i] != nullptr)
		{
			anim[i]->stop();
		}

		anim[i] = new QVariantAnimation(this);
		anim[i]->setDuration(500);
		anim[i]->setEasingCurve(QEasingCurve(QEasingCurve::Type::InOutCirc));
		anim[i]->setStartValue(mLevels[i]);
		anim[i]->setEndValue(value);
		anim[i]->setDirection(QVariantAnimation::Direction::Forward);

		connect(anim[i], QOverload<const QVariant &>::of(&QVariantAnimation::valueChanged), [this, i](const QVariant &v) {
			mLevels[i] = v.toFloat();
			repaint();
		});

		anim[i]->start();

	}
	else
	{
		mLevels[i] = value;
		repaint();
	}
}

double LiquidEqualizerWidget::getBand(int i)
{
	return mActualLevels[i];
}

void LiquidEqualizerWidget::setBands(const QVector<double> &vector,
                                     bool                   animate)
{
	int availableBandCount = (vector.count() < BANDS_NUM) ? vector.count() - 1 : BANDS_NUM;;

	for (int i = 0; i < availableBandCount; i++)
	{
		setBand(i, vector.at(i), animate);
	}

	repaint();
}

QVector<double> LiquidEqualizerWidget::getBands()
{
	QVector<double> vector;

	for (int i = 0; i < BANDS_NUM; i++)
	{
		vector.push_back(mActualLevels[i]);
	}

	return vector;
}
