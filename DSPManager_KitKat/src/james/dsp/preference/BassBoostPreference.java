package james.dsp.preference;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.DialogPreference;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import james.dsp.R;
import james.dsp.activity.Utils;
import james.dsp.activity.WM8994;

public class BassBoostPreference extends DialogPreference {

    private static final String TAG = "BassBoostPreference";

    public static final int MAX_VALUE_GAIN = 12;
    public static final int MAX_VALUE_RANGE = 5;

    public static final String BASS_BOOST_PREF_GAIN = "dsp.wm8994.bassboost.gain";
    public static final String BASS_BOOST_PREF_RANGE = "dsp.wm8994.bassboost.range";
    public static final String[] BASS_BOOST_FILES = new String[] {
        "/sys/class/misc/voodoo_sound/digital_gain",
        "/sys/class/misc/voodoo_sound/headphone_eq_b1_gain",
        "/sys/class/misc/voodoo_sound/headphone_eq_b2_gain",
    };

    public static final String[] BASS_BOOST_PREFS = {
        BASS_BOOST_PREF_GAIN,
        BASS_BOOST_PREF_RANGE,
    };
    private static final int[] SEEKBAR_ID = {
        R.id.bass_boost_gain_seekbar,
        R.id.bass_boost_range_seekbar,
    };
    private static final int[] VALUE_DISPLAY_ID = {
        R.id.bass_boost_gain_value,
        R.id.bass_boost_range_value,
    };
    private static final int[] SEEKBAR_MAX_VALUE = {
        MAX_VALUE_GAIN,
        MAX_VALUE_RANGE,
    };
    private static final String[] SEEKBAR_UOM = {
        "dB",
        "%",
    };

    private BassBoostSeekBar[] mSeekBars = new BassBoostSeekBar[2];

    private static int sInstances = 0;

    // Intermediate values for writing bass value while sliding
    private int mGain;
    private int mRange;
    private String mPreset;

    public BassBoostPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setDialogLayoutResource(R.layout.preference_dialog_bass_boost);

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        mPreset = sharedPrefs.getString(WM8994.BASS_BOOST_PRESET_PREF, "0");
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        sInstances++;
        for (int i = 0; i < SEEKBAR_ID.length; i++) {
            SeekBar seekBar = (SeekBar) view.findViewById(SEEKBAR_ID[i]);
            TextView valueDisplay = (TextView) view.findViewById(VALUE_DISPLAY_ID[i]);
            mSeekBars[i] = new BassBoostSeekBar(seekBar, valueDisplay,
                    BASS_BOOST_PREFS[i], SEEKBAR_MAX_VALUE[i], SEEKBAR_UOM[i]);
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        sInstances--;

        if (positiveResult) {
            for (BassBoostSeekBar csb : mSeekBars) {
                csb.save();
            }
        } else if (sInstances == 0) {
            BassBoostPreference.writeBassBoost(getContext());
        }
    }

    public static void restore(Context context) {
        writeBassBoost(context);
    }

    public static void writeBassBoost(Context context) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        int gain = sharedPrefs.getInt(BASS_BOOST_PREF_GAIN, MAX_VALUE_GAIN);
        int range = sharedPrefs.getInt(BASS_BOOST_PREF_RANGE, MAX_VALUE_RANGE);
        String preset = sharedPrefs.getString(WM8994.BASS_BOOST_PRESET_PREF, "0");
        writeBassBoost(gain, range, preset);
    }

    public static void writeBassBoost(Context context, String preset) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        int gain = sharedPrefs.getInt(BASS_BOOST_PREF_GAIN, MAX_VALUE_GAIN);
        int range = sharedPrefs.getInt(BASS_BOOST_PREF_RANGE, MAX_VALUE_RANGE);
        writeBassBoost(gain, range, preset);
    }

    public static void writeBassBoost(int gain, int range, String preset) {
        int mGain1 = 1;
        int mGain2 = 1;
        if (preset.equals("0")) {
            mGain2 = 0;
        } else {
            mGain1 = -1;
        }

        double digitalGain = (range / 5) * (gain * 1000);
        Utils.writeValue(BASS_BOOST_FILES[0], String.valueOf(digitalGain * -1));
        Utils.writeValue(BASS_BOOST_FILES[1], String.valueOf(gain * mGain1));
        Utils.writeValue(BASS_BOOST_FILES[2], String.valueOf(gain * mGain2));
    }

    class BassBoostSeekBar implements SeekBar.OnSeekBarChangeListener {

        private String mPref;
        private int mOriginal;
        private SeekBar mSeekBar;
        private TextView mValueDisplay;
        private int mSeekbarMax;
        private String mSeekbarUnit;

        public BassBoostSeekBar(SeekBar seekBar, TextView valueDisplay,
                String pref, int maxValue, String uom) {
            mSeekBar = seekBar;
            mValueDisplay = valueDisplay;
            mPref = pref;
            mSeekbarMax = maxValue;
            mSeekbarUnit = uom;

            SharedPreferences sharedPreferences = getSharedPreferences();
            mOriginal = sharedPreferences.getInt(mPref, mSeekbarMax);

            mSeekBar.setMax(mSeekbarMax);
            mSeekBar.setProgress(mOriginal);
            updateValue(mOriginal, false);

            mSeekBar.setOnSeekBarChangeListener(this);
        }

        public void save() {
            int value = mSeekBar.getProgress();
            Editor editor = getEditor();
            editor.putInt(mPref, value);
            editor.commit();
        }

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            updateValue(progress, true);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        private void updateValue(int progress, boolean write) {
            double mProgress = (double) progress;

            if (mPref.equals(BASS_BOOST_PREF_RANGE)) {
                mProgress = (mProgress / 5) * 100;
                mRange = progress;
            } else{
                mGain = progress;
            }

            if (write) {
                BassBoostPreference.writeBassBoost(mGain, mRange, mPreset);
            }

            mValueDisplay.setText(String.format("%d", (int) mProgress) + mSeekbarUnit);
        }

    }
}
