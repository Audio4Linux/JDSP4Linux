package james.dsp.preference;

import android.content.Context;
import android.preference.EditTextPreference;
import android.util.AttributeSet;

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
    	try
    	{
            valueFloat = Float.parseFloat(value);
    	}
    	catch(NumberFormatException e)
    	{
    	}
    	if(key.equals("dsp.masterswitch.limthreshold"))
    	{
            if(valueFloat < -60.0f)
            	valueFloat = -60.0f;
            if(valueFloat > -0.09f)
            	valueFloat = -0.09f;
            setSummary(valueFloat+" dB");
            value = Float.toString(valueFloat);
    	}
    	if(key.equals("dsp.masterswitch.limrelease"))
    	{
            if(valueFloat < 1.5f)
            	valueFloat = 1.5f;
            if(valueFloat > 2000.0f)
            	valueFloat = 2000.0f;
            setSummary(valueFloat+" ms");
            value = Float.toString(valueFloat);
    	}
    	if(key.equals("dsp.compression.threshold"))
    	{
            if(valueFloat < -80.0f)
            	valueFloat = -80;
            if(valueFloat > 0.0f)
            	valueFloat = 0.0f;
            setSummary(valueFloat+" dB");
            value = Integer.toString((int)valueFloat);
    	}
    	if(key.equals("dsp.compression.knee"))
    	{
            if(valueFloat < 0.0f)
            	value = "0";
            if(valueFloat > 40.0f)
            	value = "40";
            setSummary(valueFloat+" dB");
    	}
    	if(key.equals("dsp.compression.ratio"))
    	{
    		if(valueFloat == 0.0f)
    			value = "1";
            if(valueFloat < -20.0f)
            	value = "-20";
            if(valueFloat > 20.0f)
            	value = "20";
            setSummary("1:"+value);
    	}
    	if(key.equals("dsp.compression.attack") || key.equals("dsp.compression.release"))
    	{
            if(valueFloat < 0.00001f)
            	value = "0.00001";
            if(valueFloat > 0.99999f)
            	value = "0.99999";
            setSummary(value);
    	}
    	if(key.equals("dsp.streq.stringp"))
    	{
            if(key.length() < 12)
            	value = "GraphicEQ: 0.0 0.0; ";
    	}
    	if(key.equals("dsp.convolver.gain"))
    	{
            if(valueFloat < -80.0f)
            {
            	value = "-80.0";
            	valueFloat = -80.0f;
            }
            if(valueFloat > 30.0f)
            {
            	value = "30.0";
            	valueFloat = 30.0f;
            }
            setSummary(Float.toString(valueFloat));
    	}
    	if(key.equals("dsp.bass.freq"))
    	{
            if(valueFloat < 30.0f)
            	value = "30";
            if(valueFloat > 300.0f)
            	value = "300";
            setSummary(value+"Hz");
    	}
    	if(key.equals("dsp.analogmodelling.tubedrive") || key.equals("dsp.wavechild670.compdrive"))
    	{
            if(valueFloat < 0)
            	value = "0";
            if(valueFloat > 12)
            	value = "12";
            setSummary(value);
    	}
        super.setText(value);
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
