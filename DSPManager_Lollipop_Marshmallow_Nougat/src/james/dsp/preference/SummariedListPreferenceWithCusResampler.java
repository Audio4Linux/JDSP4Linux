package james.dsp.preference;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.media.audiofx.AudioEffect;
import android.os.Environment;
import android.preference.ListPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.Toast;

import james.dsp.R;
import james.dsp.activity.DSPManager;
import james.dsp.activity.JdspImpResToolbox;
import james.dsp.service.HeadsetService;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Locale;
import java.util.UUID;

public class SummariedListPreferenceWithCusResampler extends ListPreference
{
    public SummariedListPreferenceWithCusResampler(Context context, AttributeSet set)
    {
        super(context, set);
    }

    // Read file list from path
    public static void getFileNameList(File path, String fileExt, ArrayList<String> fileList)
    {
        if (path.isDirectory())
        {
            File[] files = path.listFiles();
            if (null == files) return;
            for (File file : files) getFileNameList(file, fileExt, fileList);
        }
        else
        {
            String filePath = path.getAbsolutePath();
            String fileName = filePath.substring(filePath.lastIndexOf("/") + 1);
            if (fileName.toLowerCase(Locale.US).endsWith(fileExt))
                fileList.add(fileName);
        }
    }
    @Override
    protected void onPrepareDialogBuilder(Builder builder)
    {
        try
        {
            if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                Log.i(DSPManager.TAG, "External storage not mounted");
                setEntries(new String[0]);
                setEntryValues(new String[0]);
                String tip = getContext().getResources().getString(R.string.text_ir_dir_isempty);
                tip = String.format(tip, DSPManager.impulseResponsePath);
                Toast.makeText(getContext(), tip, Toast.LENGTH_LONG).show();
                super.onPrepareDialogBuilder(builder);
                return;
            }
            final String kernelPath = DSPManager.impulseResponsePath;
            File kernelFile = new File(kernelPath);
            if (!kernelFile.exists())
            {
                Log.i(DSPManager.TAG, "Impulse response directory does not exists");
                kernelFile.mkdirs();
                kernelFile.mkdir();
            }
            ArrayList<String> kernelList = new ArrayList<String>();
            getFileNameList(kernelFile, ".irs", kernelList);
            getFileNameList(kernelFile, ".wav", kernelList);
            if (kernelList.isEmpty())
            {
                String tip = getContext().getResources().getString(R.string.text_ir_dir_isempty);
                tip = String.format(tip, DSPManager.impulseResponsePath);
                Toast.makeText(getContext(), tip, Toast.LENGTH_LONG).show();
            }
            else Collections.sort(kernelList);
            final String[] kernelArray = new String[kernelList.size()];
            final String[] arrayValue = new String[kernelList.size()];
            for (int i = 0; i < kernelList.size(); i++)
            {
                kernelArray[i] = kernelList.get(i);
                arrayValue[i] = kernelPath + kernelList.get(i);
            }
            setEntries(kernelArray);
            setEntryValues(arrayValue);
            super.onPrepareDialogBuilder(builder);
        }
        catch (Exception e)
        {
            setEntries(new String[0]);
            setEntryValues(new String[0]);
            String tip = getContext().getResources().getString(R.string.text_ir_dir_isempty);
            tip = String.format(tip, DSPManager.impulseResponsePath);
            Toast.makeText(getContext(), tip, Toast.LENGTH_LONG).show();
            super.onPrepareDialogBuilder(builder);
        }
    }

    @Override
    public void setValue(String value)
    {
        super.setValue(value);
        AudioEffect smprateTester;
        int samplerateNative;
        try
		{
        smprateTester = AudioEffect.class.getConstructor(UUID.class,
				UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
						HeadsetService.EFFECT_TYPE_CUSTOM, HeadsetService.EFFECT_JAMESDSP, 0, 0);
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
        try
		{
            int cmd = 20000;
			byte[] arguments = new byte[]
			{
				(byte)(cmd), (byte)(cmd >> 8),
				(byte)(cmd >> 16), (byte)(cmd >> 24)
			};
			byte[] result = new byte[4];
			Method getParameter = AudioEffect.class.getMethod("getParameter", byte[].class, byte[].class);
			getParameter.invoke(smprateTester, arguments, result);
			samplerateNative = (result[3] << 24);
			samplerateNative |= (result[2] & 0xFF) << 16;
			samplerateNative |= (result[1] & 0xFF) << 8;
			samplerateNative |= (result[0] & 0xFF);
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
        smprateTester.release();
        try
        {
            CharSequence[] entries = getEntries();
            CharSequence[] entryValues = getEntryValues();
            if (entries == null || entryValues == null)
            {
                if (value == null)
                {
                    return;
                }
                if (value.isEmpty())
                {
                    return;
                }
                if (value.contains("/"))
                {
                    String fileName = value.substring(value.lastIndexOf("/") + 1);
                    return;
                }
                return;
            }
            for (int i = 0; i < entryValues.length; i++)
            {
                if (entryValues[i].equals(value))
                {
                    JdspImpResToolbox.OfflineAudioResample(DSPManager.impulseResponsePath, entries[i].toString(), samplerateNative);
                    String finalString = DSPManager.impulseResponsePath + samplerateNative + "_" + entries[i].toString();
                    File f = new File(finalString);
                    if(f.exists() && !f.isDirectory()) {
                    	Toast.makeText(getContext(), getContext().getString(R.string.resamplerstr, samplerateNative, finalString), Toast.LENGTH_LONG).show();
                    }
                    break;
                }
            }
        }
        catch (Exception e)
        {
        }
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}
