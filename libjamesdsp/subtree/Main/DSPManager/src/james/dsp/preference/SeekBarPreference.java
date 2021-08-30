package james.dsp.preference;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import james.dsp.R;

public class SeekBarPreference extends Preference implements OnSeekBarChangeListener
{
    private static final String ANDROIDNS =
        "http://schemas.android.com/apk/res/android";
    private static final String DSP =
        "http://schemas.android.com/apk/res/james.dsp";
    private static final int DEFAULT_VALUE = 50;

    private int mMaxValue = 100;
    private int mMinValue = 0;
    private int mInterval = 1;
    private int mCurrentValue;
    private String mUnitsLeft = "";
    private String mUnitsRight = "";
    private SeekBar mSeekBar;
    private TextView mTitle;

    private TextView mStatusText;

    public SeekBarPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        initPreference(context, attrs);
    }

    public SeekBarPreference(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        initPreference(context, attrs);
    }

    private void initPreference(Context context, AttributeSet attrs)
    {
        setValuesFromXml(attrs);
        mSeekBar = new SeekBar(context, attrs);
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setOnSeekBarChangeListener(this);
    }

    private void setValuesFromXml(AttributeSet attrs)
    {
        mMaxValue = attrs.getAttributeIntValue(ANDROIDNS, "max", 100);
        mMinValue = attrs.getAttributeIntValue(DSP, "min", 0);
        mUnitsLeft = getAttributeStringValue(attrs, DSP, "unitsLeft", "");
        String units = getAttributeStringValue(attrs, DSP, "units", "");
        mUnitsRight = getAttributeStringValue(attrs, DSP, "unitsRight", units);
        try
        {
            String newInterval = attrs.getAttributeValue(DSP, "interval");
            if (newInterval != null)
                mInterval = Integer.parseInt(newInterval);
        }
        catch (Exception e)
        {
        }
    }

    private String getAttributeStringValue(AttributeSet attrs, String namespace, String name, String defaultValue)
    {
        String value = attrs.getAttributeValue(namespace, name);
        if (value == null)
            value = defaultValue;
        return value;
    }

    @Override
    public void onDependencyChanged(Preference dependency, boolean disableDependent)
    {
        super.onDependencyChanged(dependency, disableDependent);
        this.setShouldDisableView(true);
        if (mTitle != null)
            mTitle.setEnabled(!disableDependent);
        if (mSeekBar != null)
            mSeekBar.setEnabled(!disableDependent);
    }

    @Override
    protected View onCreateView(ViewGroup parent)
    {
        RelativeLayout layout = null;
        try
        {
            LayoutInflater mInflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            layout = (RelativeLayout) mInflater.inflate(R.layout.seek_bar_preference, parent, false);
            mTitle = (TextView) layout.findViewById(android.R.id.title);
        }
        catch (Exception e)
        {
        }
        return layout;
    }

    @Override
    public void onBindView(View view)
    {
        super.onBindView(view);
        try
        {
            // move our seekbar to the new view we've been given
            ViewParent oldContainer = mSeekBar.getParent();
            ViewGroup newContainer = (ViewGroup) view.findViewById(R.id.seekBarPrefBarContainer);
            if (oldContainer != newContainer)
            {
                // remove the seekbar from the old view
                if (oldContainer != null)
                    ((ViewGroup) oldContainer).removeView(mSeekBar);
                // remove the existing seekbar (there may not be one) and add ours
                newContainer.removeAllViews();
                newContainer.addView(mSeekBar, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            }
        }
        catch (Exception ex)
        {
        }
        updateView(view);
    }

    /**
     * Update a SeekBarPreference view with our current state
     *
     * @param view
     */
    protected void updateView(View view)
    {
        try
        {
            RelativeLayout layout = (RelativeLayout) view;
            mStatusText = (TextView) layout.findViewById(R.id.seekBarPrefValue);
            mStatusText.setText(String.valueOf(mCurrentValue));
            mStatusText.setMinimumWidth(30);
            mSeekBar.setProgress(mCurrentValue - mMinValue);
            TextView unitsRight = (TextView) layout.findViewById(R.id.seekBarPrefUnitsRight);
            unitsRight.setText(mUnitsRight);
            TextView unitsLeft = (TextView) layout.findViewById(R.id.seekBarPrefUnitsLeft);
            unitsLeft.setText(mUnitsLeft);
        }
        catch (Exception e)
        {
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
    {
        int newValue = progress + mMinValue;
        if (newValue > mMaxValue)
            newValue = mMaxValue;
        else if (newValue < mMinValue)
            newValue = mMinValue;
        else if (mInterval != 1 && newValue % mInterval != 0)
            newValue = Math.round(((float) newValue) / mInterval) * mInterval;
        // change rejected, revert to the previous value
        if (!callChangeListener(newValue))
        {
            seekBar.setProgress(mCurrentValue - mMinValue);
            return;
        }
        // change accepted, store it
        mCurrentValue = newValue;
        mStatusText.setText(String.valueOf(newValue));
        persistInt(newValue);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar)
    {
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar)
    {
        notifyChanged();
    }

    @Override
    protected Object onGetDefaultValue(TypedArray ta, int index)
    {
        return ta.getInt(index, DEFAULT_VALUE);
    }

    @Override
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue)
    {
        if (restoreValue)
            mCurrentValue = getPersistedInt(mCurrentValue);
        else
        {
            int temp = 0;
            try
            {
                temp = (Integer) defaultValue;
            }
            catch (Exception ex)
            {
            }
            persistInt(temp);
            mCurrentValue = temp;
        }
    }

    public void setValue(int value)
    {
        mCurrentValue = value;
    }
}
