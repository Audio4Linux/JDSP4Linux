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

public class DRCPreference extends DialogPreference
{
    protected static final String TAG = DRCPreference.class.getSimpleName();

    protected DRCSurface mDRC, mDialogDRC;

    private HeadsetService mHeadsetService;

    private final ServiceConnection connectionForDialog = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder)
        {
            mHeadsetService = ((HeadsetService.LocalBinder) binder).getService();
            updateDspFromDialogDRC();
        }
        @Override
        public void onServiceDisconnected(ComponentName name)
        {
            mHeadsetService = null;
        }
    };

    public DRCPreference(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        setLayoutResource(R.layout.drc);
        setDialogLayoutResource(R.layout.drc_popup);
    }

    protected void updateDspFromDialogDRC()
    {
        if (mHeadsetService != null)
        {
            double[] inputs = new double[15];
            double[] outputs = new double[15];
            for (int i = 0; i < outputs.length; i++)
            {
            	inputs[i] = mDialogDRC.getInput(i);
            	outputs[i] = mDialogDRC.getOutput(i);
            }
            mHeadsetService.setDRCLevels(inputs, outputs);
        }
    }

    private void updateListDRCFromValue()
    {
        String value = getPersistedString(null);
        if (value != null && mDRC != null)
        {
            String[] levelsStr = value.split(";");
//    		Log.e("DRC", "levelsStr: " + Arrays.toString(levelsStr));
            for (int i = 0; i < 15; i++)
            {
            	mDRC.setInput(i, Float.valueOf(levelsStr[i]));
                mDRC.setOutput(i, Float.valueOf(levelsStr[i + 15]));
            }
        }
    }

    @Override
    protected void onBindDialogView(View view)
    {
        super.onBindDialogView(view);
        mDialogDRC = (DRCSurface) view.findViewById(R.id.IOGraph);
        mDialogDRC.setOnTouchListener(new OnTouchListener()
        {
            @Override
            public boolean onTouch(View v, MotionEvent event)
            {
                float x = event.getX();
                float y = event.getY();
                /* Which band is closest to the position user pressed? */
                int band = mDialogDRC.findClosest(x);
                int wx = v.getWidth();
                int wy = v.getHeight();
                float levelInput = (x / wx) * 24000.0f;
                if (levelInput < 0.0f)
                	levelInput = 0.0f;
                if (levelInput > 24000.0f)
                	levelInput = 24000.0f;
                float levelOutput = (y / wy) * (DRCSurface.MIN_DB - DRCSurface.MAX_DB) - DRCSurface.MIN_DB;
                if (levelOutput < DRCSurface.MIN_DB)
                	levelOutput = DRCSurface.MIN_DB;
                if (levelOutput > DRCSurface.MAX_DB)
                	levelOutput = DRCSurface.MAX_DB;
//                mDialogDRC.setInput(band, levelInput);
                mDialogDRC.setOutput(band, levelOutput);
                updateDspFromDialogDRC();
                return true;
            }
        });
        if (mDRC != null)
        {
            for (int i = 0; i < 15; i++)
            {
                mDialogDRC.setInput(i, mDRC.getInput(i));
                mDialogDRC.setOutput(i, mDRC.getOutput(i));
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
            		value += String.format(Locale.ROOT, "%.7f", mDialogDRC.getInput(i)) + ";";
            	else
            		value += String.format(Locale.ROOT, "%.7f", mDialogDRC.getOutput(i - 15)) + ";";
            }
            persistString(value);
            updateListDRCFromValue();
//    		Log.e("DRC", "Save variable: " + value);
        }
        if (mHeadsetService != null)
            mHeadsetService.setDRCLevels(null, null);
        getContext().unbindService(connectionForDialog);
    }

    @Override
    protected void onBindView(View view)
    {
        super.onBindView(view);
        mDRC = (DRCSurface) view.findViewById(R.id.IOGraph);
        updateListDRCFromValue();
    }

    @Override
    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue)
    {
        String value = restorePersistedValue ? getPersistedString(null) : (String) defaultValue;
//		Log.e("DRC", "Load variable: " + value);
        if (shouldPersist())
            persistString(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
