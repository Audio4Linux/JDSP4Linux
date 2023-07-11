package james.dsp.preference;

import java.util.Arrays;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Paint.Cap;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.Shader;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.res.ResourcesCompat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import james.dsp.R;
import james.dsp.activity.DSPManager;
import james.dsp.activity.JdspImpResToolbox;
import james.dsp.service.HeadsetService;

public class CompanderSurface extends SurfaceView
{
    private static int MIN_FRCompander = 21;
    private static int MAX_FRCompander = 20000;
    public static float MIN_DB = -1.2f;
    public static float MAX_DB = 1.2f;

    private int mWidth;
    private int mHeight;

    private double[] mLevels = new double[7];
    private double[] mFreq = {95.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3400.0f, 7500.0f};
    private final Paint mWhite, mGridLines, mControlBarText, mControlBar, mControlBarKnob;
    private final Paint mFrequencyResponseBg, mFrequencyResponseHighlight, mFrequencyResponseHighlight2;
    int nPts = 128;
    double dispFreq[] = new double[nPts];
    float response[] = new float[nPts];
    private float[] precomputeCurveXAxis = new float[nPts];
    private float[] precomputeFreqAxis = new float[2];
    float[] addElement(float[] org, float added)
    {
    	float[] result = Arrays.copyOf(org, org.length +1);
        result[org.length] = added;
        return result;
    }
    public CompanderSurface(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        setWillNotDraw(false);
        for (int i = 0; i < nPts; i ++)
        	dispFreq[i] = reverseProjectX(i / (float)(nPts - 1));
        for (int i = 0; i < nPts; i++)
        	precomputeCurveXAxis[i] = projectX(dispFreq[i]);
        for (int freq = MIN_FRCompander; freq < MAX_FRCompander; )
        {
        	precomputeFreqAxis = addElement(precomputeFreqAxis, projectX(freq));
            if (freq < 100)
                freq += 10;
            else if (freq < 1000)
                freq += 100;
            else if (freq < 10000)
                freq += 1000;
            else
                freq += 10000;
        }
        mWhite = new Paint();
        mWhite.setColor(ContextCompat.getColor(context,R.color.white));
        mWhite.setStyle(Style.STROKE);
        mWhite.setTextSize(20);
        mWhite.setAntiAlias(true);
        mGridLines = new Paint();
        mGridLines.setColor(ContextCompat.getColor(context,R.color.grid_lines));
        mGridLines.setStyle(Style.STROKE);
        mControlBarText = new Paint(mWhite);
        mControlBarText.setTextAlign(Paint.Align.CENTER);
        mControlBarText.setShadowLayer(2, 0, 0, ContextCompat.getColor(context,R.color.cb));
        mControlBar = new Paint();
        mControlBar.setStyle(Style.STROKE);
        mControlBar.setColor(ContextCompat.getColor(context,R.color.cb));
        mControlBar.setAntiAlias(true);
        mControlBar.setStrokeCap(Cap.ROUND);
        mControlBar.setShadowLayer(2, 0, 0, ContextCompat.getColor(context,R.color.black));
        mControlBarKnob = new Paint();
        mControlBarKnob.setStyle(Style.FILL);
        mControlBarKnob.setColor(ContextCompat.getColor(context,R.color.white));
        mControlBarKnob.setAntiAlias(true);
        mFrequencyResponseBg = new Paint();
        mFrequencyResponseBg.setStyle(Style.FILL);
        mFrequencyResponseBg.setAntiAlias(true);
        mFrequencyResponseHighlight = new Paint();
        mFrequencyResponseHighlight.setStyle(Style.STROKE);
        mFrequencyResponseHighlight.setStrokeWidth(6);
        mFrequencyResponseHighlight.setColor(ContextCompat.getColor(context,R.color.freq_hl));
        mFrequencyResponseHighlight.setAntiAlias(true);
        mFrequencyResponseHighlight2 = new Paint();
        mFrequencyResponseHighlight2.setStyle(Style.STROKE);
        mFrequencyResponseHighlight2.setStrokeWidth(3);
        mFrequencyResponseHighlight2.setColor(ContextCompat.getColor(context,R.color.freq_hl2));
        mFrequencyResponseHighlight2.setAntiAlias(true);
    }

    @Override
    protected Parcelable onSaveInstanceState()
    {
        Bundle b = new Bundle();
        b.putParcelable("super", super.onSaveInstanceState());
        b.putDoubleArray("levels", mLevels);
        return b;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable p)
    {
        Bundle b = (Bundle) p;
        super.onRestoreInstanceState(b.getBundle("super"));
        mLevels = b.getDoubleArray("levels");
    }

    @Override
    protected void onAttachedToWindow()
    {
        super.onAttachedToWindow();
        setLayerType(View.LAYER_TYPE_HARDWARE, null);
        buildLayer();
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom)
    {
        super.onLayout(changed, left, top, right, bottom);
        mWidth = right - left;
        mHeight = bottom - top;
        float barWidth = getResources().getDimension(R.dimen.bar_width);
        mControlBar.setStrokeWidth(barWidth);
        mControlBarKnob.setShadowLayer(barWidth * 0.5f, 0, 0,ResourcesCompat.getColor(getResources(), R.color.off_white, null));
        /**
         * red > +7
         * yellow > +3
         * holo_blue_bright > 0
         * holo_blue < 0
         * holo_blue_dark < 3
         */
        if (DSPManager.themeApp == 0)
            mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                                           new int[] {ResourcesCompat.getColor(getResources(), R.color.eq_reddark, null),
                                        		   ResourcesCompat.getColor(getResources(), R.color.eq_yellowdark, null),
                                        		   ResourcesCompat.getColor(getResources(), R.color.eq_holo_brightdark, null),
                                        		   ResourcesCompat.getColor(getResources(), R.color.eq_holo_bluedark, null),
                                        		   ResourcesCompat.getColor(getResources(), R.color.eq_holo_darkdark, null)
                                                     },
                                           new float[] {0, 0.2f, 0.45f, 0.6f, 1f},
                                           Shader.TileMode.CLAMP));
        else if (DSPManager.themeApp == 1)
            mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                                           new int[] {ResourcesCompat.getColor(getResources(), R.color.eq_redlight, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_yellowlight, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_brightlight, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_bluelight, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_darklight, null)
                                                     },
                                           new float[] {0, 0.2f, 0.45f, 0.6f, 1f},
                                           Shader.TileMode.CLAMP));
        else if (DSPManager.themeApp == 3)
            mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                                           new int[] {ResourcesCompat.getColor(getResources(), R.color.eq_redred, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_yellowred, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_brightred, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_bluered, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_darkred, null)
                                                     },
                                           new float[] {0, 0.2f, 0.45f, 0.6f, 1f},
                                           Shader.TileMode.CLAMP));
        else if (DSPManager.themeApp == 4)
            mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                                           new int[] {ResourcesCompat.getColor(getResources(), R.color.eq_redidea, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_yellowidea, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_brightidea, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_blueidea, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_darkidea, null)
                                                     },
                                           new float[] {0, 0.2f, 0.45f, 0.6f, 1f},
                                           Shader.TileMode.CLAMP));
        else
            mFrequencyResponseBg.setShader(new LinearGradient(0, 0, 0, mHeight,
                                           new int[] {ResourcesCompat.getColor(getResources(), R.color.eq_red, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_yellow, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_bright, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_blue, null),
                                                   ResourcesCompat.getColor(getResources(), R.color.eq_holo_dark, null)
                                                     },
                                           new float[] {0, 0.2f, 0.45f, 0.6f, 1f},
                                           Shader.TileMode.CLAMP));
        mControlBar.setShader(new LinearGradient(0, 0, 0, mHeight,
                              new int[] {ResourcesCompat.getColor(getResources(), R.color.cb_shader, null),
                                         ResourcesCompat.getColor(getResources(), R.color.cb_shader_alpha, null)
                                        },
                              new float[] {0, 1},
                              Shader.TileMode.CLAMP));
    }

    public void setInput(int i, double value)
    {
        mFreq[i] = value;
        postInvalidate();
    }
    public void setOutput(int i, double value)
    {
        mLevels[i] = value;
        postInvalidate();
    }

    public double getInput(int i)
    {
        return mFreq[i];
    }
    public double getOutput(int i)
    {
        return mLevels[i];
    }

    @Override
    protected void onDraw(Canvas canvas)
    {
        /* clear canvas */
        canvas.drawRGB(0, 0, 0);
        Path freqResponse = new Path();
        int ret = JdspImpResToolbox.ComputeCompResponse(7, mFreq, mLevels, nPts, dispFreq, response);
        float x, y;
        for (int i = 0; i < nPts; i++)
        {
            /* Magnitude response, dB */
            x = precomputeCurveXAxis[i] * mWidth;
            y = projectY(response[i]) * mHeight;
            /* Set starting point at first point */
            if (i == 0)
                freqResponse.moveTo(x, y);
            else
                freqResponse.lineTo(x, y);
        }
        for (int i = 0; i < mLevels.length; i++)
        {
            x = projectX(mFreq[i]) * mWidth;
            y = projectY((float)mLevels[i]) * mHeight;
            canvas.drawLine(x, mHeight, x, y, mControlBar);
            canvas.drawCircle(x, y, mControlBar.getStrokeWidth() * 0.66f, mControlBarKnob);
            if (mFreq[i] >= 1000.0)
                canvas.drawText(String.format("%.1f", mFreq[i] / 1000.0), x, mWhite.getTextSize(), mControlBarText);
            else
                canvas.drawText(String.format("%.1f", mFreq[i]), x, mWhite.getTextSize(), mControlBarText);
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
        for (int i = 0; i < precomputeFreqAxis.length; i++)
            x = precomputeFreqAxis[i] * mWidth;
        // draw horizontal lines
        for (float dB = MIN_DB + 0.2f; dB <= MAX_DB - 0.2f; dB += 0.2f)
        {
            y = projectY(dB) * mHeight;
            canvas.drawLine(0, y, mWidth - 1, y, mGridLines);
            canvas.drawText(String.format("%+f", dB), 1, (y - 1), mWhite);
        }
    }

    private float projectX(double freq)
    {
    	double pos = Math.log(freq);
        double minPos = Math.log(MIN_FRCompander);
        double maxPos = Math.log(MAX_FRCompander);
        return (float)(((pos - minPos) / (maxPos - minPos)));
    }
    
    private double reverseProjectX(float pos) {
        double minPos = Math.log(MIN_FRCompander);
        double maxPos = Math.log(MAX_FRCompander);
        return Math.exp(pos * (maxPos - minPos) + minPos);
    }

    private float projectY(float dB)
    {
        float pos = (dB - MIN_DB) / (MAX_DB - MIN_DB);
        return (1.0f - pos);
    }

    /**
     * Find the closest control to given horizontal pixel for adjustment
     *
     * @param px
     * @return index of best match
     */
    public int findClosest(float px)
    {
        int idx = 0;
        double best = 1e8;
        for (int i = 0; i < mLevels.length; i++)
        {
        	double freq = 15.625 * Math.pow(2.6, i+1);
            double cx = projectX(freq) * mWidth;
            double distance = Math.abs(cx - px);
            if (distance < best)
            {
                idx = i;
                best = distance;
            }
        }
        return idx;
    }
}