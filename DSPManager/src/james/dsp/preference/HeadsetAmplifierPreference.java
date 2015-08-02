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

public class HeadsetAmplifierPreference extends DialogPreference {
    private static final String TAG = "HeadsetAmplifierPreference";

    private static final int MAX_VALUE = 62;
    private static final int OFFSET_VALUE = 57;

    public static final String FILE_PATH = "/sys/class/misc/voodoo_sound/headphone_amplifier_level";

    private static int sInstances = 0;

    private HeadsetAmplifierSeekBar mSeekBar;

    public HeadsetAmplifierPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setDialogLayoutResource(R.layout.preference_dialog_headphone_amplifier_level);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        sInstances++;

        SeekBar seekBar = (SeekBar) view.findViewById(R.id.headphone_amplifier_level_seekbar);
        TextView valueDisplay = (TextView) view.findViewById(R.id.headphone_amplifier_level_value);
        mSeekBar = new HeadsetAmplifierSeekBar(seekBar, valueDisplay, FILE_PATH);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        sInstances--;

        if (positiveResult) {
            mSeekBar.save();
        } else if (sInstances == 0) {
            mSeekBar.reset();
        }
    }

    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        String sDefaultValue = Utils.readOneLine(FILE_PATH);
        int value = sharedPrefs.getInt(FILE_PATH, Integer.valueOf(sDefaultValue));
        Utils.writeValue(FILE_PATH, String.valueOf(value));
    }

    public static boolean isSupported() {
        return Utils.fileExists(FILE_PATH);
    }

    private class HeadsetAmplifierSeekBar implements SeekBar.OnSeekBarChangeListener {
        private String mFilePath;
        private int mOriginal;
        private SeekBar mSeekBar;
        private TextView mValueDisplay;

        public HeadsetAmplifierSeekBar(SeekBar seekBar, TextView valueDisplay, String filePath) {
            mSeekBar = seekBar;
            mValueDisplay = valueDisplay;
            mFilePath = filePath;

            // Read original value
            SharedPreferences prefs = getSharedPreferences();
            mOriginal = prefs.getInt(mFilePath, Integer.valueOf(Utils.readOneLine(mFilePath)));

            mSeekBar.setMax(MAX_VALUE);
            reset();
            mSeekBar.setOnSeekBarChangeListener(this);
        }

        public void reset() {
            mSeekBar.setProgress(mOriginal);
            updateValue(mOriginal);
        }

        public void save() {
            int value = mSeekBar.getProgress();
            Editor editor = getEditor();
            editor.putInt(mFilePath, value);
            editor.commit();
        }

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            Utils.writeValue(FILE_PATH, String.valueOf(progress));
            updateValue(progress);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        private void updateValue(int progress) {
            mValueDisplay.setText((progress - OFFSET_VALUE) + " dB");
        }
    }
}
