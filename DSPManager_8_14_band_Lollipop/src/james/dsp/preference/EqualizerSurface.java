package james.dsp.preference;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Paint.Cap;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.Shader;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.view.SurfaceView;
import android.view.View;

import james.dsp.R;

public class EqualizerSurface extends SurfaceView {
    private static int MIN_FREQ = 16;
    private static int MAX_FREQ = 24000;
    public static int MIN_DB = -24;
    public static int MAX_DB = 24;

    private int mWidth;
    private int mHeight;

    /* Fixme: generalize with frequencies read from equalizer object */
    private float[] mLevels = new float[14];
    private final Paint mWhite, mGridLines, mControlBarText, mControlBar, mControlBarKnob;
    private final Paint mFrequencyResponseBg, mFrequencyResponseHighlight, mFrequencyResponseHighlight2;

    public EqualizerSurface(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        setWillNotDraw(false);

        mWhite = new Paint();
        mWhite.setColor(getResources().getColor(R.color.white));
        mWhite.setStyle(Style.STROKE);
        mWhite.setTextSize(20);
        mWhite.setAntiAlias(true);

        mGridLines = new Paint();
        mGridLines.setColor(getResources().getColor(R.color.grid_lines));
        mGridLines.setStyle(Style.STROKE);

        mControlBarText = new Paint(mWhite);
        mControlBarText.setTextAlign(Paint.Align.CENTER);
        mControlBarText.setShadowLayer(2, 0, 0, getResources()
                .getColor(R.color.cb));

        mControlBar = new Paint();
        mControlBar.setStyle(Style.STROKE);
        mControlBar.setColor(getResources().getColor(R.color.cb));
        mControlBar.setAntiAlias(true);
        mControlBar.setStrokeCap(Cap.ROUND);
        mControlBar.setShadowLayer(2, 0, 0, getResources()
                .getColor(R.color.black));

        mControlBarKnob = new Paint();
        mControlBarKnob.setStyle(Style.FILL);
        mControlBarKnob.setColor(getResources()
                .getColor(R.color.white));
        mControlBarKnob.setAntiAlias(true);

        mFrequencyResponseBg = new Paint();
        mFrequencyResponseBg.setStyle(Style.FILL);
        mFrequencyResponseBg.setAntiAlias(true);

        mFrequencyResponseHighlight = new Paint();
        mFrequencyResponseHighlight.setStyle(Style.STROKE);
        mFrequencyResponseHighlight.setStrokeWidth(6);
        mFrequencyResponseHighlight.setColor(getResources()
                .getColor(R.color.freq_hl));
        mFrequencyResponseHighlight.setAntiAlias(true);

        mFrequencyResponseHighlight2 = new Paint();
        mFrequencyResponseHighlight2.setStyle(Style.STROKE);
        mFrequencyResponseHighlight2.setStrokeWidth(3);
        mFrequencyResponseHighlight2.setColor(getResources()
                .getColor(R.color.freq_hl2));
        mFrequencyResponseHighlight2.setAntiAlias(true);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Bundle b = new Bundle();
        b.putParcelable("super", super.onSaveInstanceState());
        b.putFloatArray("levels", mLevels);
        return b;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable p) {
        Bundle b = (Bundle) p;
        super.onRestoreInstanceState(b.getBundle("super"));
        mLevels = b.getFloatArray("levels");
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        setLayerType(View.LAYER_TYPE_HARDWARE, null);
        buildLayer();
    }

    /**
     * Returns a color that is assumed to be blended against black background,
     * assuming close to sRGB behavior of screen (gamma 2.2 approximation).
     *
     * @param intensity desired physical intensity of color component
     * @param alpha     alpha value of color component
     */
    private static int gamma(float intensity, float alpha) {
        /* intensity = (component * alpha)^2.2
         * <=>
         * intensity^(1/2.2) / alpha = component
         */

        return (int) Math.min(255, Math.max(0, Math.round(255 * Math.pow(intensity, 1 / 2.2) / alpha)));
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        final Resources res = getResources();
        mWidth = right - left;
        mHeight = bottom - top;

        float barWidth = res.getDimension(R.dimen.bar_width);
        mControlBar.setStrokeWidth(barWidth);
        mControlBarKnob.setShadowLayer(barWidth * 0.5f, 0, 0,
                res.getColor(R.color.off_white));
        mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                /**
                 * red > +7
                 * yellow > +3
                 * holo_blue_bright > 0
                 * holo_blue < 0
                 * holo_blue_dark < 3
                 */
                new int[]{res.getColor(R.color.eq_red),
                        res.getColor(R.color.eq_yellow),
                        res.getColor(R.color.eq_holo_bright),
                        res.getColor(R.color.eq_holo_blue),
                        res.getColor(R.color.eq_holo_dark)},
                new float[]{0, 0.2f, 0.45f, 0.6f, 1f},
                Shader.TileMode.CLAMP));
        mControlBar.setShader(new LinearGradient(0, 0, 0, mHeight,
                new int[]{res.getColor(R.color.cb_shader),
                        res.getColor(R.color.cb_shader_alpha)},
                new float[]{0, 1},
                Shader.TileMode.CLAMP));
    }

    public void setBand(int i, float value) {
        mLevels[i] = value;
        postInvalidate();
    }

    public float getBand(int i) {
        return mLevels[i];
    }

    @Override
    protected void onDraw(Canvas canvas) {
        /* clear canvas */
        canvas.drawRGB(0, 0, 0);

        Biquad[] biquads = new Biquad[]{
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
                new Biquad(),
        };

        /* The filtering is realized with 2nd order high shelf filters, and each band
         * is realized as a transition relative to the previous band. The center point for
         * each filter is actually between the bands.
         *
         * 1st band has no previous band, so it's just a fixed gain.
         */
        double gain = Math.pow(10, mLevels[0] / 20);
        for (int i = 0; i < biquads.length; i++) {
            if (i == 0) {
            double freq = 32.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 1) {
            double freq = 64.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 2) {
            double freq = 126.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 3) {
            double freq = 200.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 4) {
            double freq = 317.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 5) {
            double freq = 502.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 6) {
            double freq = 796.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 7) {
            double freq = 1260.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 8) {
            double freq = 2000.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 9) {
            double freq = 3170.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 10) {
            double freq = 5020.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 11) {
            double freq = 7960.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 12) {
            double freq = 13500.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else if (i == 13) {
            double freq = 18500.0;
            int SAMPLING_RATE = 48000;
            biquads[i].setHighShelf(freq * 2, SAMPLING_RATE, mLevels[i + 1] - mLevels[i], 1);
            }
            else {
            // Should not happen
            }
        }

        Path freqResponse = new Path();
        for (int i = 0; i < 71; i++) {
            double freq = reverseProjectX(i / 70f);
            int SAMPLING_RATE = 48000;
            double omega = freq / SAMPLING_RATE * Math.PI * 2;
            Complex z = new Complex(Math.cos(omega), Math.sin(omega));

            /* Evaluate the response at frequency z */

            /* Complex z1 = z.mul(gain); */
            Complex z2 = biquads[0].evaluateTransfer(z);
            Complex z3 = biquads[1].evaluateTransfer(z);
            Complex z4 = biquads[2].evaluateTransfer(z);
            Complex z5 = biquads[3].evaluateTransfer(z);
            Complex z6 = biquads[4].evaluateTransfer(z);
            Complex z7 = biquads[5].evaluateTransfer(z);
            Complex z8 = biquads[6].evaluateTransfer(z);
            Complex z9 = biquads[7].evaluateTransfer(z);
            Complex z10 = biquads[8].evaluateTransfer(z);
            Complex z11 = biquads[9].evaluateTransfer(z);
            Complex z12 = biquads[10].evaluateTransfer(z);
            Complex z13 = biquads[11].evaluateTransfer(z);
            Complex z14 = biquads[12].evaluateTransfer(z);

            /* Magnitude response, dB */
            double dB = lin2dB(gain * z2.rho() * z3.rho() * z4.rho() * z5.rho() * z6.rho() * z7.rho() * z8.rho() * z9.rho() * z10.rho() * z11.rho() * z12.rho() * z13.rho() * z14.rho());
            float x = projectX(freq) * mWidth;
            float y = projectY(dB) * mHeight;

            /* Set starting point at first point */
            if (i == 0) {
                freqResponse.moveTo(x, y);
            } else {
                freqResponse.lineTo(x, y);
            }
        }

        Path freqResponseBg = new Path();
        freqResponseBg.addPath(freqResponse);
        freqResponseBg.offset(0, -4);
        freqResponseBg.lineTo(mWidth, mHeight);
        freqResponseBg.lineTo(0, mHeight);
        freqResponseBg.close();
        canvas.drawPath(freqResponseBg, mFrequencyResponseBg);

        canvas.drawPath(freqResponse, mFrequencyResponseHighlight);
        canvas.drawPath(freqResponse, mFrequencyResponseHighlight2);

        /* Set the width of the bars according to canvas size */
        canvas.drawRect(0, 0, mWidth - 1, mHeight - 1, mWhite);

        /* draw vertical lines */
        for (int freq = MIN_FREQ; freq < MAX_FREQ; ) {
            float x = projectX(freq) * mWidth;
            canvas.drawLine(x, 0, x, mHeight - 1, mGridLines);
            if (freq < 100) {
                freq += 10;
            } else if (freq < 1000) {
                freq += 100;
            } else if (freq < 10000) {
                freq += 1000;
            } else {
                freq += 10000;
            }
        }

        /* draw horizontal lines */
        for (int dB = MIN_DB + 3; dB <= MAX_DB - 3; dB += 3) {
            float y = projectY(dB) * mHeight;
            canvas.drawLine(0, y, mWidth - 1, y, mGridLines);
            canvas.drawText(String.format("%+d", dB), 1, (y - 1), mWhite);
        }

        for (int i = 0; i < mLevels.length; i++) {
            if (i == 0) {
            double freq = 32.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 1) {
            double freq = 64.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 2) {
            double freq = 126.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 3) {
            double freq = 200.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 4) {
            double freq = 317.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 5) {
            double freq = 502.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 6) {
            double freq = 796.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 7) {
            double freq = 1260.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 8) {
            double freq = 2000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 9) {
            double freq = 3170.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 10) {
            double freq = 5020.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 11) {
            double freq = 7960.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 12) {
            double freq = 13500.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 13) {
            double freq = 18500.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText(String.format("%+1.1f", mLevels[i]), x, mHeight - 2, mControlBarText);
            canvas.drawText(String.format(freq < 1000 ? "%.0f" : "%.0fk", freq < 1000 ? freq : freq / 1000), x, mWhite.getTextSize(), mControlBarText);
            }
            else {
            // Should not happen
            }
        }
    }

    private float projectX(double freq) {
        double pos = Math.log(freq);
        double minPos = Math.log(MIN_FREQ);
        double maxPos = Math.log(MAX_FREQ);
        return (float) ((pos - minPos) / (maxPos - minPos));
    }

    private double reverseProjectX(float pos) {
        double minPos = Math.log(MIN_FREQ);
        double maxPos = Math.log(MAX_FREQ);
        return Math.exp(pos * (maxPos - minPos) + minPos);
    }

    private float projectY(double dB) {
        double pos = (dB - MIN_DB) / (MAX_DB - MIN_DB);
        return (float) (1 - pos);
    }

    private double lin2dB(double rho) {
        return rho != 0 ? Math.log(rho) / Math.log(10) * 20 : -99.9;
    }

    /**
     * Find the closest control to given horizontal pixel for adjustment
     *
     * @param px
     * @return index of best match
     */
    public int findClosest(float px) {
        int idx = 0;
        float best = 1e9f;
        for (int i = 0; i < mLevels.length; i++) {
            double freq = 20 * Math.pow(1.72, i);
            float cx = projectX(freq) * mWidth;
            float distance = Math.abs(cx - px);

            if (distance < best) {
                idx = i;
                best = distance;
            }
        }

        return idx;
    }
}
