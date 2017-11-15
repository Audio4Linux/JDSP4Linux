package james.dsp.preference;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

import james.dsp.R;
import james.dsp.service.HeadsetService;

import java.util.Locale;

public class EqualizerPreference extends DialogPreference
{
    protected static final String TAG = EqualizerPreference.class.getSimpleName();

    protected EqualizerSurface mListEqualizer, mDialogEqualizer;

    private HeadsetService mHeadsetService;

    private final ServiceConnection connectionForDialog = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder)
        {
            mHeadsetService = ((HeadsetService.LocalBinder) binder).getService();
            updateDspFromDialogEqualizer();
        }
        @Override
        public void onServiceDisconnected(ComponentName name)
        {
            mHeadsetService = null;
        }
    };

    public EqualizerPreference(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        setLayoutResource(R.layout.equalizer);
        setDialogLayoutResource(R.layout.equalizer_popup);
    }

    protected void updateDspFromDialogEqualizer()
    {
        if (mHeadsetService != null)
        {
            float[] levels = new float[15];
            for (int i = 0; i < levels.length; i++)
                levels[i] = mDialogEqualizer.getBand(i);
            mHeadsetService.setEqualizerLevels(levels);
        }
    }

    private void updateListEqualizerFromValue()
    {
        String value = getPersistedString(null);
        if (value != null && mListEqualizer != null)
        {
            String[] levelsStr = value.split(";");
            for (int i = 0; i < 15; i++)
                mListEqualizer.setBand(i, Float.valueOf(levelsStr[i]));
        }
    }

    @Override
    protected void onBindDialogView(View view)
    {
        super.onBindDialogView(view);
        mDialogEqualizer = (EqualizerSurface) view.findViewById(R.id.FrequencyResponse);
        mDialogEqualizer.setOnTouchListener(new OnTouchListener()
        {
            @Override
            public boolean onTouch(View v, MotionEvent event)
            {
                float x = event.getX();
                float y = event.getY();
                /* Which band is closest to the position user pressed? */
                int band = mDialogEqualizer.findClosest(x);
                int wy = v.getHeight();
                float level = (y / wy) * (EqualizerSurface.MIN_DB - EqualizerSurface.MAX_DB) - EqualizerSurface.MIN_DB;
                if (level < EqualizerSurface.MIN_DB)
                    level = EqualizerSurface.MIN_DB;
                if (level > EqualizerSurface.MAX_DB)
                    level = EqualizerSurface.MAX_DB;
                mDialogEqualizer.setBand(band, level);
                updateDspFromDialogEqualizer();
                return true;
            }
        });
        if (mListEqualizer != null)
        {
            for (int i = 0; i < 15; i++)
                mDialogEqualizer.setBand(i, mListEqualizer.getBand(i));
        }
        getContext().bindService(new Intent(getContext(), HeadsetService.class), connectionForDialog, 0);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult)
    {
        if (positiveResult)
        {
            String value = "";
            for (int i = 0; i < 15; i++)
                value += String.format(Locale.ROOT, "%.7f", mDialogEqualizer.getBand(i)) + ";";
            persistString(value);
            updateListEqualizerFromValue();
        }
        if (mHeadsetService != null)
            mHeadsetService.setEqualizerLevels(null);
        getContext().unbindService(connectionForDialog);
    }

    @Override
    protected void onBindView(View view)
    {
        super.onBindView(view);
        mListEqualizer = (EqualizerSurface) view.findViewById(R.id.FrequencyResponse);
        updateListEqualizerFromValue();
    }

    @Override
    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue)
    {
        String value = restorePersistedValue ? getPersistedString(null) : (String) defaultValue;
        if (shouldPersist())
            persistString(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
