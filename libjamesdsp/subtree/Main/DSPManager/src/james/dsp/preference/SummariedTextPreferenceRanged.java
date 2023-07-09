package james.dsp.preference;

import android.content.Context;
import android.preference.EditTextPreference;
import android.util.AttributeSet;
import android.util.Log;
import james.dsp.activity.DSPManager;

public class SummariedTextPreferenceRanged extends EditTextPreference
{
    public SummariedTextPreferenceRanged(Context context, AttributeSet set)
    {
        super(context, set);
    }
    @Override
    public void setText(String value)
    {
    	String key = getKey();
    	float valueFloat = 0.0f;
		if (key.equals("dsp.streq.stringp") || key.equals("dsp.convolver.advimp"))
		{
	        super.setText(value);
			return;
		}
    	try
    	{
    		valueFloat = Float.parseFloat(value);
    	}
    	catch(NumberFormatException e)
    	{
			Log.e(DSPManager.TAG, "SummariedTextPreferenceRanged found invalid string: " + value);
    	}
    	if(key.equals("dsp.masterswitch.limthreshold"))
    	{
            if(valueFloat < -60.0f)
            	valueFloat = -60.0f;
            if(valueFloat > -0.09f)
            	valueFloat = -0.09f;
            setSummary(valueFloat + " dB");
            value = Float.toString(valueFloat);
    	}
    	if(key.equals("dsp.masterswitch.limrelease"))
    	{
            if(valueFloat < 1.5f)
            	valueFloat = 1.5f;
            if(valueFloat > 2000.0f)
            	valueFloat = 2000.0f;
            setSummary(valueFloat + " ms");
            value = Float.toString(valueFloat);
    	}
    	if(key.equals("dsp.masterswitch.postgain"))
    	{
            if(valueFloat < -15.0f)
            	valueFloat = -15.0f;
            if(valueFloat > 15.0f)
            	valueFloat = 15.0f;
            setSummary(valueFloat + " dB");
            value = Float.toString(valueFloat);
    	}
    	if(key.equals("dsp.streq.stringp"))
    	{
            if(key.length() < 12)
            	value = "GraphicEQ: 0.0 0.0; ";
    	}
    	if(key.equals("dsp.analogmodelling.tubedrive"))
    	{
            if(valueFloat < -3.0f)
            	valueFloat = -3.0f;
            if(valueFloat > 8.0f)
            	valueFloat = 8.0f;
            setSummary(valueFloat + " dB");
            value = Float.toString(valueFloat);
    	}
        super.setText(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
