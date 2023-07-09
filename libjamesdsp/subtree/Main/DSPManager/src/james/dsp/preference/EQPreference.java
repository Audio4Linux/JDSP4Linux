package james.dsp.preference;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

import james.dsp.R;
import james.dsp.service.HeadsetService;

import java.util.Arrays;
import java.util.Locale;

public class EQPreference extends DialogPreference
{
    protected static final String TAG = EQPreference.class.getSimpleName();

    protected EQSurface mEQ, mDialogEQ;

    private HeadsetService mHeadsetService;

    private final ServiceConnection connectionForDialog = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder)
        {
            mHeadsetService = ((HeadsetService.LocalBinder) binder).getService();
            updateDspFromDialogEQ();
        }
        @Override
        public void onServiceDisconnected(ComponentName name)
        {
            mHeadsetService = null;
        }
    };

    public EQPreference(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        setLayoutResource(R.layout.eq);
        setDialogLayoutResource(R.layout.eq_popup);
    }

    protected void updateDspFromDialogEQ()
    {
        if (mHeadsetService != null)
        {
            double[] inputs = new double[15];
            double[] outputs = new double[15];
            for (int i = 0; i < outputs.length; i++)
            {
            	inputs[i] = mDialogEQ.getInput(i);
            	outputs[i] = mDialogEQ.getOutput(i);
            }
            mHeadsetService.setEQLevels(inputs, outputs);
        }
    }

    private void updateListEQFromValue()
    {
        String value = getPersistedString(null);
        if (value != null && mEQ != null)
        {
            String[] levelsStr = value.split(";");
//    		Log.e("EQ", "levelsStr: " + Arrays.toString(levelsStr));
            for (int i = 0; i < 15; i++)
            {
            	mEQ.setInput(i, Float.valueOf(levelsStr[i]));
                mEQ.setOutput(i, Float.valueOf(levelsStr[i + 15]));
            }
        }
    }

    @Override
    protected void onBindDialogView(View view)
    {
        super.onBindDialogView(view);
        mDialogEQ = (EQSurface) view.findViewById(R.id.MagnitudeResponse);
        mDialogEQ.setOnTouchListener(new OnTouchListener()
        {
            @Override
            public boolean onTouch(View v, MotionEvent event)
            {
                float x = event.getX();
                float y = event.getY();
                /* Which band is closest to the position user pressed? */
                int band = mDialogEQ.findClosest(x);
                int wx = v.getWidth();
                int wy = v.getHeight();
                float levelInput = (x / wx) * 24000.0f;
                if (levelInput < 0.0f)
                	levelInput = 0.0f;
                if (levelInput > 24000.0f)
                	levelInput = 24000.0f;
                float levelOutput = (y / wy) * (EQSurface.MIN_DB - EQSurface.MAX_DB) - EQSurface.MIN_DB;
                if (levelOutput < EQSurface.MIN_DB)
                	levelOutput = EQSurface.MIN_DB;
                if (levelOutput > EQSurface.MAX_DB)
                	levelOutput = EQSurface.MAX_DB;
//                mDialogEQ.setInput(band, levelInput);
                mDialogEQ.setOutput(band, levelOutput);
                updateDspFromDialogEQ();
                return true;
            }
        });
        if (mEQ != null)
        {
            for (int i = 0; i < 15; i++)
            {
                mDialogEQ.setInput(i, mEQ.getInput(i));
                mDialogEQ.setOutput(i, mEQ.getOutput(i));
            }
        }
        getContext().bindService(new Intent(getContext(), HeadsetService.class), connectionForDialog, 0);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult)
    {
        if (positiveResult)
        {
            String value = "";
            for (int i = 0; i < 30; i++)
            {
            	if (i < 15)
            		value += String.format(Locale.ROOT, "%.7f", mDialogEQ.getInput(i)) + ";";
            	else
            		value += String.format(Locale.ROOT, "%.7f", mDialogEQ.getOutput(i - 15)) + ";";
            }
            persistString(value);
            updateListEQFromValue();
//    		Log.e("EQ", "Save variable: " + value);
        }
        if (mHeadsetService != null)
            mHeadsetService.setEQLevels(null, null);
        getContext().unbindService(connectionForDialog);
    }

    @Override
    protected void onBindView(View view)
    {
        super.onBindView(view);
        mEQ = (EQSurface) view.findViewById(R.id.MagnitudeResponse);
        updateListEQFromValue();
    }

    @Override
    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue)
    {
        String value = restorePersistedValue ? getPersistedString(null) : (String) defaultValue;
//		Log.e("EQ", "Load variable: " + value);
        if (shouldPersist())
            persistString(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
