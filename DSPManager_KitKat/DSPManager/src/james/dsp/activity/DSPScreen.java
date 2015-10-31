package james.dsp.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

import james.dsp.R;
import james.dsp.preference.EqualizerPreference;
import james.dsp.preference.SummariedListPreference;
import james.dsp.service.HeadsetService;

/**
 * This class implements a general PreferencesActivity that we can use to
 * adjust DSP settings. It adds a menu to clear the preferences on this page,
 * and a listener that ensures that our {@link HeadsetService} is running if
 * required.
 *
 * @author alankila
 */
public final class DSPScreen extends PreferenceFragment {
    protected static final String TAG = DSPScreen.class.getSimpleName();

    public static final String PREF_KEY_EQ = "dsp.tone.eq";
    public static final String PREF_KEY_CUSTOM_EQ = "dsp.tone.eq.custom";

    public static final String EQ_VALUE_CUSTOM = "custom";

    private final OnSharedPreferenceChangeListener listener =
            new OnSharedPreferenceChangeListener() {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
            /* If the listpref is updated, copy the changed setting to the eq. */
            if (PREF_KEY_EQ.equals(key)) {
                String newValue = prefs.getString(key, null);
                if (!EQ_VALUE_CUSTOM.equals(newValue)) {
                    prefs.edit().putString(PREF_KEY_CUSTOM_EQ, newValue).commit();

                    /* Now tell the equalizer that it must display something else. */
                    EqualizerPreference eq =
                            (EqualizerPreference) findPreference(PREF_KEY_CUSTOM_EQ);
                    eq.refreshFromPreference();
                }
            }

            /* If the equalizer surface is updated, select matching pref entry or "custom". */
            if (PREF_KEY_CUSTOM_EQ.equals(key)) {
                String newValue = prefs.getString(key, null);

                String desiredValue = EQ_VALUE_CUSTOM;
                SummariedListPreference preset =
                        (SummariedListPreference) findPreference(PREF_KEY_EQ);

                for (CharSequence entry : preset.getEntryValues()) {
                    if (entry.equals(newValue)) {
                        desiredValue = newValue;
                        break;
                    }
                }

                /* Tell listpreference that it must display something else. */
                if (!desiredValue.equals(preset.getEntry())) {
                    prefs.edit().putString(PREF_KEY_EQ, desiredValue).commit();
                    preset.refreshFromPreference();
                }
            }

            getActivity().sendBroadcast(new Intent(DSPManager.ACTION_UPDATE_PREFERENCES));
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String config = getArguments().getString("config");
        PreferenceManager prefMgr = getPreferenceManager();

        prefMgr.setSharedPreferencesName(DSPManager.SHARED_PREFERENCES_BASENAME + "." + config);

        try {
            int xmlId = R.xml.class.getField(config + "_preferences").getInt(null);
            addPreferencesFromResource(xmlId);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            throw new RuntimeException(e);
        }

        prefMgr.getSharedPreferences().registerOnSharedPreferenceChangeListener(listener);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.getSharedPreferences().unregisterOnSharedPreferenceChangeListener(listener);
    }
}
