package james.dsp.activity;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

import james.dsp.R;
import james.dsp.preference.BassBoostPreference;
import james.dsp.preference.HeadsetAmplifierPreference;

public class WM8994 extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    protected static final String TAG = WM8994.class.getSimpleName();

    public static final String NAME = "wm8994";
    public static final String WM8994_ENABLE_FILE = "/sys/class/misc/voodoo_sound_control/enable";
    public static final String VOODOO_SOUND_PACKAGE = "org.projectvoodoo.controlapp";

    public static final String BASS_BOOST_ENABLE_PREF = "dsp.wm8994.bassboost.enable";
    public static final String BASS_BOOST_PRESET_PREF = "dsp.wm8994.bassboost.preset";
    public static final String BASS_BOOST_GAIN_RANGE_PREF = "dsp.wm8994.bassboost.gainrange";
    public static final String BASS_BOOST_ENABLE_FILE = "/sys/class/misc/voodoo_sound/headphone_eq";

    public static final String[][] OPTION_CONTROLS = {
        {"/sys/class/misc/voodoo_sound/speaker_tuning", "pref_wm8994_speaker_tuning"},
        {"/sys/class/misc/voodoo_sound/mono_downmix", "pref_wm8994_mono_downmix"},
        {"/sys/class/misc/voodoo_sound/stereo_expansion", "pref_wm8994_stereo_expansion"},
        {"/sys/class/misc/voodoo_sound/dac_direct", "pref_wm8994_dac_direct"},
        {"/sys/class/misc/voodoo_sound/dac_osr128", "pref_wm8994_dac_osr128"},
        {"/sys/class/misc/voodoo_sound/adc_osr128", "pref_wm8994_adc_osr128"},
        {"/sys/class/misc/voodoo_sound/fll_tuning", "pref_wm8994_fll_tuning"},
        {BASS_BOOST_ENABLE_FILE, BASS_BOOST_ENABLE_PREF},
    };

    public static final String MIC_REC_PRESET[][] = {
        {"/sys/class/misc/voodoo_sound/recording_preset", "dsp.wm8994.microphone.recording"}
    };

    private static final String PREF_ENABLED = "1";
    private static final String PREF_DISABLED = "0";

    private CheckBoxPreference mPreferences[] = new CheckBoxPreference[OPTION_CONTROLS.length];

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.wm8994_preferences);

        PreferenceScreen prefSet = getPreferenceScreen();

        for (int i = 0; i < OPTION_CONTROLS.length;i++) {
            String fileName = OPTION_CONTROLS[i][0];
            mPreferences[i] = (CheckBoxPreference) prefSet.findPreference(OPTION_CONTROLS[i][1]);
            if (Utils.fileExists(fileName)) {
                mPreferences[i].setChecked(PREF_ENABLED.equals(Utils.readOneLine(fileName)));
                mPreferences[i].setOnPreferenceChangeListener(this);
            } else {
                mPreferences[i].setSummary(R.string.pref_unavailable);
                mPreferences[i].setEnabled(false);
            }
        }

        if (Utils.fileExists(HeadsetAmplifierPreference.FILE_PATH)) {
            prefSet.findPreference("headphone_amp").setOnPreferenceChangeListener(this);
        } else {
            Preference category = prefSet.findPreference("wm8994_headphone_amp_category");
            prefSet.removePreference(category);
        }

        if (Utils.fileExists(MIC_REC_PRESET[0][0])) {
            prefSet.findPreference(MIC_REC_PRESET[0][1]).setOnPreferenceChangeListener(this);
        } else {
            Preference category = prefSet.findPreference("wm8994_microphone_recording_category");
            prefSet.removePreference(category);
        }

        Preference bassBoostPreset = prefSet.findPreference(BASS_BOOST_PRESET_PREF);
        Preference bassBoostGainRange = prefSet.findPreference(BASS_BOOST_GAIN_RANGE_PREF);
        if (Utils.fileExists(BASS_BOOST_ENABLE_FILE)) {
            bassBoostPreset.setOnPreferenceChangeListener(this);
            bassBoostGainRange.setOnPreferenceChangeListener(this);
        } else {
            PreferenceCategory bassBoostCategory =
                    (PreferenceCategory) prefSet.findPreference("wm8994_signal_processing_category");
            bassBoostCategory.removePreference(bassBoostPreset);
            bassBoostCategory.removePreference(bassBoostGainRange);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();

        if (MIC_REC_PRESET[0][1].equals(key)) {
            Utils.writeValue(MIC_REC_PRESET[0][0], (String) newValue);
        } else if (BASS_BOOST_PRESET_PREF.equals(key)) {
            BassBoostPreference.writeBassBoost(getActivity(), (String) newValue);
        } else {
            for (String[] pair : OPTION_CONTROLS) {
                if (pair[1].equals(key)) {
                    if ((Boolean) newValue) {
                        Utils.writeValue(pair[0], PREF_ENABLED);
                    } else {
                        Utils.writeValue(pair[0], PREF_DISABLED);
                    }
                }
            }
        }

        return true; // true for automatic update in preference file
    }

    /**
     * Check if WM8994 is supported on this phone
     */
    public static boolean isSupported(Context context) {
        boolean hasVoodoo = true;

        // Try to detect presence of Voodoo Sound app and disable our control
        // to prevent conflict.
        try {
            PackageManager pm = context.getPackageManager();
            pm.getPackageInfo(VOODOO_SOUND_PACKAGE, 0);
        } catch (NameNotFoundException e) {
            hasVoodoo = false;
        }

        return !hasVoodoo && Utils.fileExists(WM8994_ENABLE_FILE);
    }

    public static void restore(Context context) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);

        for (String[] pair : OPTION_CONTROLS) {
            if (Utils.fileExists(pair[0])) {
                Utils.writeValue(pair[0], sharedPrefs.getBoolean(pair[1],
                        PREF_ENABLED.equals(Utils.readOneLine(pair[0]))));
            }
        }

        if (Utils.fileExists(MIC_REC_PRESET[0][0])) {
            Utils.writeValue(MIC_REC_PRESET[0][0], sharedPrefs.getString(MIC_REC_PRESET[0][1],
                        Utils.readOneLine(MIC_REC_PRESET[0][0])));
        }

        if (Utils.fileExists(BASS_BOOST_ENABLE_FILE)) {
            BassBoostPreference.restore(context);
        }

        HeadsetAmplifierPreference.restore(context);
    }
}
