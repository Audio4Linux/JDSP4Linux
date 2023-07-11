package james.dsp.activity;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import james.dsp.R;
import james.dsp.preference.CompanderPreference;
import james.dsp.preference.EQPreference;
import james.dsp.preference.SummariedListPreference;
import james.dsp.service.HeadsetService;

/**
 * This class implements a general PreferencesActivity that we can use to
 * adjust DSP settings. It adds a menu to clear the preferences on this page,
 * and a listener that ensures that our {@link HeadsetService} is running if
 * required.
 *
 * @Co-founder alankila
 */
public final class DSPScreen extends PreferenceFragment
{
    protected static final String TAG = DSPScreen.class.getSimpleName();

    private final OnSharedPreferenceChangeListener listener = new OnSharedPreferenceChangeListener()
    {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
        {
            /* If the listpref is updated, copy the changed setting to the eq. */
            if ("dsp.tone.eq".equals(key))
            {
                String newValue = sharedPreferences.getString(key, null);
                if (!"custom".equals(newValue))
                {
                    Editor e = sharedPreferences.edit();
                    e.putString("dsp.tone.eq.custom", newValue);
                    e.commit();
                    /* Now tell the equalizer that it must display something else. */
                    EQPreference eq = (EQPreference)getPreferenceScreen().findPreference("dsp.tone.eq.custom");
                    eq.refreshFromPreference();
                }
            }
            /* If the equalizer surface is updated, select matching pref entry or "custom". */
            if ("dsp.tone.eq.custom".equals(key))
            {
                String newValue = sharedPreferences.getString(key, null);
                String desiredValue = "custom";
                SummariedListPreference preset = (SummariedListPreference)getPreferenceScreen().findPreference("dsp.tone.eq");
                for (CharSequence entry : preset.getEntryValues())
                {
                    if (entry.equals(newValue))
                    {
                        desiredValue = newValue;
                        break;
                    }
                }
                /* Tell listpreference that it must display something else. */
                if (!desiredValue.equals(preset.getEntry()))
                {
                    Editor e = sharedPreferences.edit();
                    e.putString("dsp.tone.eq", desiredValue);
                    e.commit();
                    preset.refreshFromPreference();
                }
            }
            /* If the listpref is updated, copy the changed setting to the eq. */
            if ("dsp.compression.eq".equals(key))
            {
                String newValue = sharedPreferences.getString(key, null);
                if (!"custom".equals(newValue))
                {
                    Editor e = sharedPreferences.edit();
                    e.putString("dsp.compression.eq.custom", newValue);
                    e.commit();
                    /* Now tell the equalizer that it must display something else. */
                    CompanderPreference eq = (CompanderPreference)getPreferenceScreen().findPreference("dsp.compression.eq.custom");
                    eq.refreshFromPreference();
                }
            }
            /* If the equalizer surface is updated, select matching pref entry or "custom". */
            if ("dsp.compression.eq.custom".equals(key))
            {
                String newValue = sharedPreferences.getString(key, null);
                String desiredValue = "custom";
                SummariedListPreference preset = (SummariedListPreference)getPreferenceScreen().findPreference("dsp.compression.eq");
                for (CharSequence entry : preset.getEntryValues())
                {
                    if (entry.equals(newValue))
                    {
                        desiredValue = newValue;
                        break;
                    }
                }
                /* Tell listpreference that it must display something else. */
                if (!desiredValue.equals(preset.getEntry()))
                {
                    Editor e = sharedPreferences.edit();
                    e.putString("dsp.compression.eq", desiredValue);
                    e.commit();
                    preset.refreshFromPreference();
                }
            }
            if (!"dsp.convolver.resampler".equals(key))
            	getActivity().sendBroadcast(new Intent(DSPManager.ACTION_UPDATE_PREFERENCES));
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        String config = getArguments().getString("config");
        getPreferenceManager().setSharedPreferencesName(
            DSPManager.SHARED_PREFERENCES_BASENAME + "." + config);
        getPreferenceManager().setSharedPreferencesMode(Context.MODE_MULTI_PROCESS);
        try
        {
            int xmlId = R.xml.class.getField(config + "_preferences").getInt(null);
            addPreferencesFromResource(xmlId);
        }
        catch (Exception e)
        {
            throw new RuntimeException(e);
        }
        getPreferenceManager().getSharedPreferences()
        .registerOnSharedPreferenceChangeListener(listener);
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        getPreferenceManager().getSharedPreferences()
        .unregisterOnSharedPreferenceChangeListener(listener);
    }
}