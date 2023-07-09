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

public class CompanderPreference extends DialogPreference
{
    protected static final String TAG = CompanderPreference.class.getSimpleName();

    protected CompanderSurface mDynamicResponse, mDialogDynamicResponse;

    private HeadsetService mHeadsetService;

    private final ServiceConnection connectionForDialog = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder)
        {
            mHeadsetService = ((HeadsetService.LocalBinder) binder).getService();
            updateDspFromDialogDynamicResponse();
        }
        @Override
        public void onServiceDisconnected(ComponentName name)
        {
            mHeadsetService = null;
        }
    };

    public CompanderPreference(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        setLayoutResource(R.layout.compander);
        setDialogLayoutResource(R.layout.compander_popup);
    }

    protected void updateDspFromDialogDynamicResponse()
    {
        if (mHeadsetService != null)
        {
            double[] inputs = new double[7];
            double[] outputs = new double[7];
            for (int i = 0; i < outputs.length; i++)
            {
            	inputs[i] = mDialogDynamicResponse.getInput(i);
            	outputs[i] = mDialogDynamicResponse.getOutput(i);
            }
            mHeadsetService.setCompLevels(inputs, outputs);
        }
    }

    private void updateListDynamicResponseFromValue()
    {
        String value = getPersistedString(null);
        if (value != null && mDynamicResponse != null)
        {
            String[] levelsStr = value.split(";");
//    		Log.e("DynamicResponse", "levelsStr: " + Arrays.toString(levelsStr));
            for (int i = 0; i < 7; i++)
            {
            	mDynamicResponse.setInput(i, Float.valueOf(levelsStr[i]));
                mDynamicResponse.setOutput(i, Float.valueOf(levelsStr[i + 7]));
            }
        }
    }

    @Override
    protected void onBindDialogView(View view)
    {
        super.onBindDialogView(view);
        mDialogDynamicResponse = (CompanderSurface) view.findViewById(R.id.DynamicResponse);
        mDialogDynamicResponse.setOnTouchListener(new OnTouchListener()
        {
            @Override
            public boolean onTouch(View v, MotionEvent event)
            {
                float x = event.getX();
                float y = event.getY();
                /* Which band is closest to the position user pressed? */
                int band = mDialogDynamicResponse.findClosest(x);
                int wx = v.getWidth();
                int wy = v.getHeight();
                float levelInput = (x / wx) * 24000.0f;
                if (levelInput < 0.0f)
                	levelInput = 0.0f;
                if (levelInput > 24000.0f)
                	levelInput = 24000.0f;
                float levelOutput = (y / wy) * (CompanderSurface.MIN_DB - CompanderSurface.MAX_DB) - CompanderSurface.MIN_DB;
                if (levelOutput < CompanderSurface.MIN_DB)
                	levelOutput = CompanderSurface.MIN_DB;
                if (levelOutput > CompanderSurface.MAX_DB)
                	levelOutput = CompanderSurface.MAX_DB;
//                mDialogDynamicResponse.setInput(band, levelInput);
                mDialogDynamicResponse.setOutput(band, levelOutput);
                updateDspFromDialogDynamicResponse();
                return true;
            }
        });
        if (mDynamicResponse != null)
        {
            for (int i = 0; i < 7; i++)
            {
                mDialogDynamicResponse.setInput(i, mDynamicResponse.getInput(i));
                mDialogDynamicResponse.setOutput(i, mDynamicResponse.getOutput(i));
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
            for (int i = 0; i < 14; i++)
            {
            	if (i < 7)
            		value += String.format(Locale.ROOT, "%.7f", mDialogDynamicResponse.getInput(i)) + ";";
            	else
            		value += String.format(Locale.ROOT, "%.7f", mDialogDynamicResponse.getOutput(i - 7)) + ";";
            }
            persistString(value);
            updateListDynamicResponseFromValue();
//    		Log.e("DynamicResponse", "Save variable: " + value);
        }
        if (mHeadsetService != null)
            mHeadsetService.setCompLevels(null, null);
        getContext().unbindService(connectionForDialog);
    }

    @Override
    protected void onBindView(View view)
    {
        super.onBindView(view);
        mDynamicResponse = (CompanderSurface) view.findViewById(R.id.DynamicResponse);
        updateListDynamicResponseFromValue();
    }

    @Override
    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue)
    {
        String value = restorePersistedValue ? getPersistedString(null) : (String) defaultValue;
//		Log.e("DynamicResponse", "Load variable: " + value);
        if (shouldPersist())
            persistString(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
