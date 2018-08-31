package james.dsp.preference;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.os.Environment;
import android.preference.ListPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.Toast;

import james.dsp.R;
import james.dsp.activity.DSPManager;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Locale;

public class SummariedListPreferenceDDC extends ListPreference
{
    public SummariedListPreferenceDDC(Context context, AttributeSet set)
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
                String tip = getContext().getResources().getString(R.string.text_ddc_dir_isempty);
                tip = String.format(tip, DSPManager.ddcPath);
                Toast.makeText(getContext(), tip, Toast.LENGTH_LONG).show();
                super.onPrepareDialogBuilder(builder);
                return;
            }
            final String kernelPath = DSPManager.ddcPath;
            File kernelFile = new File(kernelPath);
            if (!kernelFile.exists())
            {
                Log.i(DSPManager.TAG, "Impulse response directory does not exists");
                kernelFile.mkdirs();
                kernelFile.mkdir();
            }
            ArrayList<String> kernelList = new ArrayList<String>();
            getFileNameList(kernelFile, ".vdc", kernelList);
            if (kernelList.isEmpty())
            {
                String tip = getContext().getResources().getString(R.string.text_ddc_dir_isempty);
                tip = String.format(tip, DSPManager.ddcPath);
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
            String tip = getContext().getResources().getString(R.string.text_ddc_dir_isempty);
            tip = String.format(tip, DSPManager.ddcPath);
            Toast.makeText(getContext(), tip, Toast.LENGTH_LONG).show();
            super.onPrepareDialogBuilder(builder);
        }
    }

    @Override
    public void setValue(String value)
    {
        super.setValue(value);
        try
        {
            CharSequence[] entries = getEntries();
            CharSequence[] entryValues = getEntryValues();
            if (entries == null || entryValues == null)
            {
                if (value == null)
                {
                    setSummary("");
                    return;
                }
                if (value.isEmpty())
                {
                    setSummary("");
                    return;
                }
                if (value.contains("/"))
                {
                    String fileName = value.substring(value.lastIndexOf("/") + 1);
                    setSummary(fileName);
                    return;
                }
                setSummary(value);
                return;
            }
            for (int i = 0; i < entryValues.length; i++)
            {
                if (entryValues[i].equals(value))
                {
                    setSummary(entries[i]);
                    break;
                }
            }
        }
        catch (Exception e)
        {
            setSummary("");
        }
    }

    public void refreshFromPreference()
    {
        onSetInitialValue(true, null);
    }
}