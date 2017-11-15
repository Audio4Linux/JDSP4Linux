package james.dsp.preference;

import android.content.Context;
import android.preference.EditTextPreference;
import android.util.AttributeSet;
import james.dsp.R;
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
    	try
    	{
            valueFloat = Float.parseFloat(value);
    	}
    	catch(NumberFormatException e)
    	{
    	}
    	if(key.equals("dsp.masterswitch.finalgain") || key.equals("dsp.headphone.tailverb"))
    	{
            if(valueFloat < 1.0f)
            	value = "1";
            if(valueFloat > 200.0f)
            	value = "200";
            setSummary(value);
    	}
    	if(key.equals("dsp.compression.threshold"))
    	{
            if(valueFloat < -80.0f)
            	valueFloat = -80;
            if(valueFloat > 0.0f)
            	valueFloat = 0.0f;
            setSummary(value+" dB");
            value = Integer.toString((int)valueFloat);
    	}
    	if(key.equals("dsp.compression.knee"))
    	{
            if(valueFloat < 0.0f)
            	value = "0";
            if(valueFloat > 40.0f)
            	value = "40";
            setSummary(value+" dB");
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
    	if(key.equals("dsp.convolver.normalise"))
    	{
            if(valueFloat < 0.000001f)
            {
            	value = "0.000001";
            	valueFloat = 0.000001f;
            }
            if(valueFloat > 0.99999f)
            {
            	value = "0.99999";
            	valueFloat = 0.99999f;
            }
            setSummary(Float.toString(valueFloat*100)+"%");
    	}
    	if(key.equals("dsp.bass.freq"))
    	{
            if(valueFloat < 30.0f)
            	value = "30";
            if(valueFloat > 300.0f)
            	value = "300";
            setSummary(value+"Hz");
    	}
    	if(key.equals("dsp.headphone.roomsize") || key.equals("dsp.headphone.reverbtime"))
    	{
            if(valueFloat < 5.0f)
            	value = "5";
            if(valueFloat > 300.0f)
            	value = "300";
            setSummary(value+"m");
    	}
    	if(key.equals("dsp.headphone.damping"))
    	{
            if(valueFloat < 1)
            	value = "1";
            if(valueFloat > 100)
            	value = "100";
            setSummary(value+"%");
    	}
    	if(key.equals("dsp.headphone.inbandwidth"))
    	{
            if(valueFloat < 1)
            	value = "1";
            if(valueFloat > 60)
            	value = "60";
            setSummary(value+DSPManager.actUi.getString(R.string.hfcomponents));
    	}
    	if(key.equals("dsp.headphone.earlyverb"))
    	{
            if(valueFloat < 1)
            	value = "1";
            if(valueFloat > 100)
            	value = "100";
            setSummary(value);
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
