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
    public static int MIN_DB = -18;
    public static int MAX_DB = 18;

    private int mWidth;
    private int mHeight;

    private float[] mLevels = new float[10];
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
        Path freqResponse = new Path();
	        for (int i = 0; i < mLevels.length; i++) {
            if (i == 0) {
			float x = projectX(0.0001) * mWidth;
			float y = projectY(mLevels[i]) * mHeight;
			freqResponse.moveTo(x, y);
            x = projectX(31.0) * mWidth;
            freqResponse.lineTo(x, y);
            }
            else if (i == 1) {
            float x = projectX(62.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            freqResponse.lineTo(x, y);
            }
            else if (i == 2) {
            float x = projectX(125.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 3) {
            float x = projectX(250.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 4) {
            float x = projectX(500.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 5) {
            float x = projectX(1000.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 6) {
            float x = projectX(2000.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 7) {
            float x = projectX(4000.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 8) {
            float x = projectX(8000.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else if (i == 9) {
            float x = projectX(16000.0) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
			x = projectX(MAX_FREQ) * mWidth;
            y = projectY(mLevels[i]) * mHeight;
			freqResponse.lineTo(x, y);
            }
            else {
            // Should not happen
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

        // Set the width of the bars according to canvas size
        canvas.drawRect(0, 0, mWidth - 1, mHeight - 1, mWhite);

        // draw vertical lines
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

        // draw horizontal lines
        for (int dB = MIN_DB + 3; dB <= MAX_DB - 3; dB += 3) {
            float y = projectY(dB) * mHeight;
            canvas.drawLine(0, y, mWidth - 1, y, mGridLines);
            canvas.drawText(String.format("%+d", dB), 1, (y - 1), mWhite);
        }

        for (int i = 0; i < mLevels.length; i++) {
            if (i == 0) {
            double freq = 31.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("31", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 1) {
            double freq = 62.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("62", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 2) {
            double freq = 125.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("125", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 3) {
            double freq = 250.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("250", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 4) {
            double freq = 500.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("500", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 5) {
            double freq = 1000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("1000", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 6) {
            double freq = 2000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("2k", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 7) {
            double freq = 4000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("4k", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 8) {
            double freq = 8000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("8k", x, mWhite.getTextSize(), mControlBarText);
            }
            else if (i == 9) {
            double freq = 16000.0;
            float x = projectX(freq) * mWidth;
            float y = projectY(mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            canvas.drawText("16k", x, mWhite.getTextSize(), mControlBarText);
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

    private float projectY(double dB) {
        double pos = (dB - MIN_DB) / (MAX_DB - MIN_DB);
        return (float) (1 - pos);
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
            double freq = 15.625 * Math.pow(2, i+1);
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