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
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */

#ifndef QFLUENTEQUALIZERWIDGET_H
#define QFLUENTEQUALIZERWIDGET_H

#define BANDS_NUM 15
#define MIN_DB -12
#define MAX_DB 12
#define MIN_FREQ 25.0
#define MAX_FREQ 24000.0
#define SAMPLING_RATE (MAX_FREQ * 2)

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVariantAnimation>
#include "misc/biquad.h"

const static QVector<float> mFreq({25.0f, 40.0f, 63.0f, 100.0f, 160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f});
const static QVector<float> mFreqAlt({25.0f, 40.0f, 63.0f, 100.0f, 160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f});

class LiquidEqualizerWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool gridVisible READ getGridVisible WRITE setGridVisible NOTIFY redrawRequired)
    Q_PROPERTY(bool alwaysDrawHandles READ getAlwaysDrawHandles WRITE setAlwaysDrawHandles NOTIFY redrawRequired)
    Q_PROPERTY(QColor accentColor READ getAccentColor WRITE setAccentColor NOTIFY redrawRequired)
    Q_PROPERTY(int animationDuration READ getAnimationDuration WRITE setAnimationDuration NOTIFY redrawRequired)
    public:
    LiquidEqualizerWidget(QWidget *parent = nullptr);
    ~LiquidEqualizerWidget();
    void setBand(int i, float value, bool animate = true, bool manual = false);
    float getBand(int i);
    void setBands(const QVector<float>& vector, bool animate = true);
    QVector<float> getBands();

    bool getGridVisible() const;
    void setGridVisible(bool gridVisible);
    bool getAlwaysDrawHandles() const;
    void setAlwaysDrawHandles(bool alwaysDrawHandles);
    QColor getAccentColor() const;
    void setAccentColor(const QColor &accentColor);
    int getAnimationDuration() const;
    void setAnimationDuration(int animationDuration);

signals:
    void mouseReleased();
    void redrawRequired();
    void bandsUpdated();
    void cancelAnimations();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
private:
    QVariantAnimation* anim[BANDS_NUM];

    QPainter mGridLines;
    QPainter mControlBarText;
    QPainter mFrequencyResponseBg;
    QPainter mFrequencyResponseHighlight;
    QPainter mControlBarKnob;
    biquad   biquads[BANDS_NUM] = {};
    float    mLevels[BANDS_NUM] = {};
    int      mHeight = 0;
    int      mWidth = 0;
    int      mSelectedBand = 0;
    bool     mManual = false;
    bool     mHoldDown = false;

    int      mKey_CurrentIndex;
    QPoint   mKey_LastMousePos;

    bool     mGridVisible = true;
    bool     mAlwaysDrawHandles = false;
    QColor   mAccentColor = QColor(98,0,238,255);
    int      mAnimationDuration = 500;

    inline double reverseProjectX(double position) {
        double minimumPosition = log(MIN_FREQ);
        double maximumPosition = log(MAX_FREQ);
        return exp(position * (maximumPosition - minimumPosition) + minimumPosition);
    }

    inline float projectX(double frequency) {
        double position = log(frequency);
        double minimumPosition = log(MIN_FREQ);
        double maximumPosition = log(MAX_FREQ);
        return ((position - minimumPosition) / (maximumPosition - minimumPosition));
    }

    inline float projectY(float dB) {
        float pos = (dB - MIN_DB) / (MAX_DB - MIN_DB);
        return 1.0f - pos;
    }

    inline float reverseProjectY(float pos) {
        float i = abs(pos - 1);
        return -i * MIN_DB + i * MAX_DB + MIN_DB;
    }

    inline float lin2dB(double rho) {
        if (rho != 0.0) {
            return (log(rho) / log(10.0) * 20);
        } else {
            return -99.9f;
        }
    }

    inline int getIndexUnderMouse(int pos_x){
        int index = 0;
        for (int i = BANDS_NUM - 1; i >= 0; i--) {
            double frequency = mFreq[i];
            double x = projectX(frequency) * mWidth;
            if(x < pos_x){
                index = i;
                break;
            }
        }
        return index;
    }

};
#endif // QFLUENTEQUALIZERWIDGET_H
