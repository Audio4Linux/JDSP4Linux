package james.dsp.activity;
import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v7.app.ActionBarDrawerToggle;
import android.util.Log;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v4.view.GravityCompat;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;

import james.dsp.R;
import james.dsp.service.HeadsetService;
import james.dsp.widgets.CustomDrawerLayout;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import android.widget.Toast;

/**
* This page is displays the activity_main-level configurations menu.
*/
public final class DSPManager extends Activity
{
	public static Context actUi = null;
    //==================================
    // Static Fields
    //==================================
    public static final String TAG = "JamesDSPManager";
    private static final String STATE_SELECTED_POSITION = "selected_navigation_drawer_position";
    private static final String PREF_USER_LEARNED_DRAWER = "navigation_drawer_learned";
    //==================================
    public static final String SHARED_PREFERENCES_BASENAME = "james.dsp";
    public static final String ACTION_UPDATE_PREFERENCES = "james.dsp.UPDATE";
    public static final String JAMESDSP_FOLDER = "JamesDSP";
    public static final String IMPULSERESPONSE_FOLDER = "Convolver";
    public static final String DDC_FOLDER = "DDC";
    private static final String PRESETS_FOLDER = "Presets";
    private static int routing;
    public static final String benchmarkPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + DSPManager.JAMESDSP_FOLDER + "/";
    public static final String wisdomTxt = "ConvolverWisdom.txt";
    public static final String impulseResponsePath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + DSPManager.JAMESDSP_FOLDER + "/" + DSPManager.IMPULSERESPONSE_FOLDER + "/";
    public static final String ddcPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + DSPManager.JAMESDSP_FOLDER + "/" + DSPManager.DDC_FOLDER + "/";
    public static boolean devMsgDisplay;
    //==================================
    private static String[] mEntries;
    private static List<HashMap<String, String>> mTitles;

    //==================================
    // Drawer
    //==================================
    private ActionBarDrawerToggle mDrawerToggle;
    private CustomDrawerLayout mDrawerLayout;
    private ListView mDrawerListView;
    private View mFragmentContainerView;
    private int mCurrentSelectedPosition = 0;
    private boolean mFromSavedInstanceState;
    private boolean mUserLearnedDrawer;

    //==================================
    // ViewPager
    //==================================
    protected MyAdapter pagerAdapter;
    protected ViewPager viewPager;

    //==================================
    // Fields
    //==================================
    private SharedPreferences preferencesMode;
    private CharSequence mTitle;
    public static int themeApp;
    public static int effectMode;
    //==================================
    // Overridden Methods
    //==================================
    @Override
    public Resources.Theme getTheme()
    {
        Resources.Theme theme = super.getTheme();
        if(themeApp == 0)
            theme.applyStyle(android.R.style.Theme_Material, true);
        else if(themeApp == 1)
            theme.applyStyle(android.R.style.Theme_Material_Light, true);
        else if(themeApp == 2)
            theme.applyStyle(R.style.AppTheme, true);
        else if(themeApp == 3)
            theme.applyStyle(R.style.AppThemeRed, true);
        else if(themeApp == 4)
            theme.applyStyle(R.style.AppThemeIdea, true);
        return theme;
    }
	private final BroadcastReceiver updateReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
	{
  		  String action = intent.getAction();
  		  if(action.equals("dsp.activity.updatePage")){
              if (HeadsetService.mUseBluetooth)
                  routing = 2;
              else if (HeadsetService.mUseHeadset)
                  routing = 0;
              else
                  routing = 1;
			selectItem(routing);
  		  }
	}
	};
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        actUi = getApplicationContext();
        preferencesMode = getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + "settings", 0);
        SharedPreferences preferences = getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + HeadsetService.getAudioOutputRouting(), 0);
        File impFile = new File(preferences.getString("dsp.convolver.files", ""));
        if (!impFile.exists())
            preferences.edit().putString("dsp.convolver.files", getString(R.string.doesntexist)).commit();
        File ddcFile = new File(preferences.getString("dsp.ddc.files", ""));
        if (!ddcFile.exists())
            preferences.edit().putString("dsp.ddc.files", getString(R.string.ddcdoesntexist)).commit();
        if (!preferencesMode.contains("dsp.app.showdevmsg"))
            preferencesMode.edit().putBoolean("dsp.app.showdevmsg", false).commit();
        if (!preferencesMode.contains("dsp.app.theme"))
            preferencesMode.edit().putInt("dsp.app.theme", 2).commit();
        if (!preferencesMode.contains("dsp.app.modeEffect"))
        	preferencesMode.edit().putInt("dsp.app.modeEffect", 0).commit();
        devMsgDisplay = preferencesMode.getBoolean("dsp.app.showdevmsg", false);
        themeApp = preferencesMode.getInt("dsp.app.theme", 2);
        effectMode = preferencesMode.getInt("dsp.app.modeEffect", 0);
        mUserLearnedDrawer = preferencesMode.getBoolean(PREF_USER_LEARNED_DRAWER, false);
        mTitle = getTitle();
        ActionBar mActionBar = getActionBar();
        mActionBar.setDisplayHomeAsUpEnabled(true);
        mActionBar.setHomeButtonEnabled(true);
        mActionBar.setDisplayShowTitleEnabled(true);
        if (savedInstanceState != null)
        {
            mCurrentSelectedPosition = savedInstanceState.getInt(STATE_SELECTED_POSITION);
            mFromSavedInstanceState = true;
        }
        Intent serviceIntent = new Intent(this, HeadsetService.class);
        startService(serviceIntent);
        sendBroadcast(new Intent(DSPManager.ACTION_UPDATE_PREFERENCES));
        setUpUi();
		registerReceiver(updateReceiver, new IntentFilter("dsp.activity.updatePage"));
        if (HeadsetService.mUseBluetooth)
            routing = 2;
        else if (HeadsetService.mUseHeadset)
            routing = 0;
        else
            routing = 1;
		selectItem(routing);
    }
    @Override
    protected void onResume()
    {
        super.onResume();
		registerReceiver(updateReceiver, new IntentFilter("dsp.activity.updatePage"));
    }
    @Override
    protected void onPause()
    {
        super.onPause();
		unregisterReceiver(updateReceiver);
    }
    @Override
    protected void onPostCreate(Bundle savedInstanceState)
    {
        super.onPostCreate(savedInstanceState);
        // Sync the toggle state after onRestoreInstanceState has occurred.
        mDrawerToggle.syncState();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public void onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putInt(STATE_SELECTED_POSITION, mCurrentSelectedPosition);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        if (!isDrawerOpen())
        {
            getMenuInflater().inflate(R.menu.menu, menu);
            getActionBar().setTitle(mTitle);
            return true;
        }
        else
        {
            getActionBar().setTitle(getString(R.string.app_name));
            return super.onCreateOptionsMenu(menu);
        }
    }
    @Override
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        if (!isDrawerOpen())
        {
            menu.findItem(R.id.mdisplaydevmsg).setTitle(getString(R.string.displaydevmsg, getString(devMsgDisplay ? R.string.removetext : R.string.displaytext)));
            if (themeApp == 0)
                menu.findItem(R.id.themeswitch).setTitle(getString(R.string.theme, getString(R.string.darktheme)));
            else if (themeApp == 1)
                menu.findItem(R.id.themeswitch).setTitle(getString(R.string.theme, getString(R.string.lighttheme)));
            else if (themeApp == 2)
                menu.findItem(R.id.themeswitch).setTitle(getString(R.string.theme, getString(R.string.defaulttheme)));
            else if (themeApp == 3)
                menu.findItem(R.id.themeswitch).setTitle(getString(R.string.theme, getString(R.string.redtheme)));
            else if (themeApp == 4)
                menu.findItem(R.id.themeswitch).setTitle(getString(R.string.theme, getString(R.string.ideatheme)));
            if (effectMode == 0)
                menu.findItem(R.id.globaleffectitm).setTitle(getString(R.string.globaleffect_title, getString(R.string.globalreg)));
            else
                menu.findItem(R.id.globaleffectitm).setTitle(getString(R.string.globaleffect_title, getString(R.string.traditionalreg)));
        }
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        if (mDrawerToggle.onOptionsItemSelected(item))
            return true;
        int choice = item.getItemId();
        switch (choice)
        {
        case R.id.help:
            DialogFragment df = new HelpFragment();
            df.setStyle(DialogFragment.STYLE_NO_TITLE, 0);
            df.show(getFragmentManager(), "help");
            return true;
        case R.id.save_preset:
            savePresetDialog();
            return true;
        case R.id.load_preset:
            loadPresetDialog();
            return true;
        case R.id.mdisplaydevmsg:
            devMsgDisplay = !devMsgDisplay;
            preferencesMode.edit().putBoolean("dsp.app.showdevmsg", devMsgDisplay).commit();
            return true;
        case R.id.themeswitch:
            themeApp++;
            if(themeApp > 4)
                themeApp = 0;
            preferencesMode.edit().putInt("dsp.app.theme", themeApp).commit();
            startActivity(new Intent(this, DSPManager.class));
            finish();
            return true;
        case R.id.globaleffectitm:
        	effectMode++;
        	if(effectMode > 1)
        		effectMode = 0;
        	preferencesMode.edit().putInt("dsp.app.modeEffect", effectMode).commit();
            Intent serviceIntent = new Intent(this, HeadsetService.class);
            startService(serviceIntent);
        	return true;
        default:
            return false;
        }
    }

    //==================================
    // Methods
    //==================================

    @SuppressLint("NewApi")
    private void setUpUi()
    {
        mTitles = getTitles();
        mEntries = getEntries();
        setContentView(R.layout.activity_main);
        mDrawerListView = (ListView)findViewById(R.id.dsp_navigation_drawer);
        mDrawerListView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView< ? > parent, View view, int position, long id)
            {
                selectItem(position);
            }
        });
        mDrawerListView.setAdapter(new SimpleAdapter(
                                       this,
                                       mTitles,
                                       R.layout.drawer_item,
                                       new String[] { "ICON", "TITLE" },
                                       new int[] {R.id.drawer_icon, R.id.drawer_title}));
        mDrawerListView.setItemChecked(mCurrentSelectedPosition, true);
        setUpNavigationDrawer(
            findViewById(R.id.dsp_navigation_drawer),
            findViewById(R.id.dsp_drawer_layout));
        if (themeApp == 0)
            mDrawerListView.setBackgroundColor(ResourcesCompat.getColor(getResources(), R.color.navigation_drawerdark, null));
        else if (themeApp == 1)
            mDrawerListView.setBackgroundColor(ResourcesCompat.getColor(getResources(), R.color.navigation_drawerlight, null));
        else if (themeApp == 2)
            mDrawerListView.setBackgroundColor(ResourcesCompat.getColor(getResources(), R.color.navigation_drawerdefault, null));
        else if (themeApp == 3)
            mDrawerListView.setBackgroundColor(ResourcesCompat.getColor(getResources(), R.color.navigation_drawerred, null));
        else if (themeApp == 4)
            mDrawerListView.setBackgroundColor(ResourcesCompat.getColor(getResources(), R.color.navigation_draweridea, null));
    }
    public void savePresetDialog()
    {
        // We first list existing presets
        File presetsDir = new File(Environment.getExternalStorageDirectory()
                                   .getAbsolutePath() + "/" + JAMESDSP_FOLDER + "/" + PRESETS_FOLDER);
        presetsDir.mkdirs();
        // The first entry is "New preset", so we offset
        File[] presets = presetsDir.listFiles((FileFilter)null);
        final String[] names = new String[presets != null ? presets.length + 1 : 1];
        names[0] = getString(R.string.new_preset);
        if (presets != null)
        {
            for (int i = 0; i < presets.length; i++)
                names[i + 1] = presets[i].getName();
        }
        AlertDialog.Builder builder = new AlertDialog.Builder(DSPManager.this);
        builder.setTitle(R.string.save_preset)
        .setItems(names, new DialogInterface.OnClickListener()
        {
            public void onClick(DialogInterface dialog, int which)
            {
                if (which == 0)
                {
                    // New preset, we ask for the name
                    AlertDialog.Builder inputBuilder =
                        new AlertDialog.Builder(DSPManager.this);
                    inputBuilder.setTitle(R.string.new_preset);
                    Toast.makeText(getApplicationContext(), R.string.warn_null, Toast.LENGTH_LONG).show();
                    final EditText input = new EditText(DSPManager.this);
                    inputBuilder.setView(input);
                    inputBuilder.setPositiveButton(
                        "Ok", new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int whichButton)
                        {
                            String value = input.getText().toString();
                            savePreset(value);
                        }
                    });
                    inputBuilder.setNegativeButton(
                        "Cancel", new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int whichButton)
                        {
                            // Canceled.
                        }
                    });
                    inputBuilder.show();
                }
                else
                    savePreset(names[which]);
            }
        });
        Dialog dlg = builder.create();
        dlg.show();
    }

    public void loadPresetDialog()
    {
        File presetsDir = new File(Environment.getExternalStorageDirectory()
                                   .getAbsolutePath() + "/" + JAMESDSP_FOLDER + "/" + PRESETS_FOLDER);
        presetsDir.mkdirs();
        File[] presets = presetsDir.listFiles((FileFilter)null);
        final String[] names = new String[presets != null ? presets.length : 0];
        if (presets != null)
        {
            for (int i = 0; i < presets.length; i++)
                names[i] = presets[i].getName();
        }
        AlertDialog.Builder builder = new AlertDialog.Builder(DSPManager.this);
        builder.setTitle(R.string.load_preset)
        .setItems(names, new DialogInterface.OnClickListener()
        {
            public void onClick(DialogInterface dialog, int which)
            {
                loadPreset(names[which]);
            }
        });
        builder.create().show();
    }
    /*Grant permission response code*/
    public void savePreset(String name)
    {
        final String spDir = getApplicationInfo().dataDir + "/shared_prefs/";
        // Copy the SharedPreference to our output directory
        File presetDir = new File(Environment.getExternalStorageDirectory()
                                  .getAbsolutePath() + "/" + JAMESDSP_FOLDER + "/" + PRESETS_FOLDER + "/" + name);
        presetDir.mkdirs();
        final String packageName = "james.dsp.";
        try
        {
            copy(new File(spDir + packageName + "bluetooth.xml"),
                 new File(presetDir, packageName + "bluetooth.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.save_error_bluetooth, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(spDir + packageName + "headset.xml"),
                 new File(presetDir, packageName + "headset.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.save_error_headset, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(spDir + packageName + "speaker.xml"),
                 new File(presetDir, packageName + "speaker.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.save_error_speaker, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(spDir + packageName + "settings.xml"),
                 new File(presetDir, packageName + "settings.xml"));
        }
        catch (IOException e)
        {
        }
    }

    public void loadPreset(String name)
    {
        // Copy the SharedPreference to our local directory
        File presetDir = new File(Environment.getExternalStorageDirectory()
                                  .getAbsolutePath() + "/" + JAMESDSP_FOLDER + "/" + PRESETS_FOLDER + "/" + name);
        if (!presetDir.exists()) presetDir.mkdirs();
        final String packageName = "james.dsp.";
        final String spDir = getApplicationInfo().dataDir + "/shared_prefs/";
        try
        {
            copy(new File(presetDir, packageName + "bluetooth.xml"),
                 new File(spDir + packageName + "bluetooth.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.load_error_bluetooth, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(presetDir, packageName + "headset.xml"),
                 new File(spDir + packageName + "headset.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.load_error_headset, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(presetDir, packageName + "speaker.xml"),
                 new File(spDir + packageName + "speaker.xml"));
        }
        catch (IOException e)
        {
            Toast.makeText(getApplicationContext(), R.string.load_error_speaker, Toast.LENGTH_LONG).show();
        }
        try
        {
            copy(new File(presetDir, packageName + "settings.xml"),
                 new File(spDir + packageName + "settings.xml"));
        }
        catch (IOException e)
        {
        }
        // Reload preferences
        startActivity(new Intent(this, DSPManager.class));
        finish();
    }

    public static void copy(File src, File dst) throws IOException
    {
        InputStream in = new FileInputStream(src);
        OutputStream out = new FileOutputStream(dst);
        // Transfer bytes from in to out
        byte[] buf = new byte[1024];
        int len;
        while ((len = in.read(buf)) > 0)
            out.write(buf, 0, len);
        in.close();
        out.close();
    }

    /**
    * Users of this fragment must call this method to set up the
    * navigation menu_drawer interactions.
    *
    * @param fragmentContainerView The view of this fragment in its activity's layout.
    * @param drawerLayout          The DrawerLayout containing this fragment's UI.
    */
    public void setUpNavigationDrawer(View fragmentContainerView, View drawerLayout)
    {
        mFragmentContainerView = fragmentContainerView;
        mDrawerLayout = (CustomDrawerLayout)drawerLayout;
        mDrawerLayout.setDrawerShadow(R.drawable.drawer_shadow, GravityCompat.START);
        mDrawerToggle = new ActionBarDrawerToggle(
            this,
            mDrawerLayout,
            R.string.navigation_drawer_open,
            R.string.navigation_drawer_close
        )
        {
            @Override
            public void onDrawerClosed(View drawerView)
            {
                super.onDrawerClosed(drawerView);
                invalidateOptionsMenu(); // calls onPrepareOptionsMenu()
            }
            @Override
            public void onDrawerOpened(View drawerView)
            {
                super.onDrawerOpened(drawerView);
                if (!mUserLearnedDrawer)
                {
                    mUserLearnedDrawer = true;
                    preferencesMode.edit().putBoolean(PREF_USER_LEARNED_DRAWER, true).commit();
                }
                invalidateOptionsMenu(); // calls onPrepareOptionsMenu()
            }
        };
        if (!mUserLearnedDrawer && !mFromSavedInstanceState)
            mDrawerLayout.openDrawer(mFragmentContainerView);
        mDrawerLayout.post(new Runnable()
        {
            @Override
            public void run()
            {
                mDrawerToggle.syncState();
            }
        });
        mDrawerToggle.setDrawerIndicatorEnabled(true);
        mDrawerLayout.addDrawerListener(mDrawerToggle);
        selectItem(mCurrentSelectedPosition);
    }

    public boolean isDrawerOpen()
    {
        return mDrawerLayout != null && mDrawerLayout.isDrawerOpen(mFragmentContainerView);
    }

    private void selectItem(int position)
    {
        mCurrentSelectedPosition = position;
        if (mDrawerListView != null)
            mDrawerListView.setItemChecked(position, true);
        if (mDrawerLayout != null)
            mDrawerLayout.closeDrawer(mFragmentContainerView);
        FragmentManager fragmentManager = getFragmentManager();
        fragmentManager.beginTransaction()
        .replace(R.id.dsp_container, PlaceholderFragment.newInstance(position))
        .commit();
        mTitle = mTitles.get(position).get("TITLE");
        getActionBar().setTitle(mTitle);
    }

    /**
    * @return String[] containing titles
    */
    private List<HashMap<String, String>> getTitles()
    {
        ArrayList<HashMap<String, String>> tmpList = new ArrayList<HashMap<String, String>>();
        // Headset
        HashMap<String, String> mTitleMap = new HashMap<String, String>();
        mTitleMap.put("ICON", R.drawable.empty_icon + "");
        mTitleMap.put("TITLE", getString(R.string.headset_title));
        tmpList.add(mTitleMap);
        // Speaker
        mTitleMap = new HashMap<String, String>();
        mTitleMap.put("ICON", R.drawable.empty_icon + "");
        mTitleMap.put("TITLE", getString(R.string.speaker_title));
        tmpList.add(mTitleMap);
        // Bluetooth
        mTitleMap = new HashMap<String, String>();
        mTitleMap.put("ICON", R.drawable.empty_icon + "");
        mTitleMap.put("TITLE", getString(R.string.bluetooth_title));
        tmpList.add(mTitleMap);
        return tmpList;
    }

    /**
    * @return String[] containing titles
    */
    private String[] getEntries()
    {
        ArrayList<String> entryString = new ArrayList<String>();
        entryString.add("headset");
        entryString.add("speaker");
        entryString.add("bluetooth");
        return entryString.toArray(new String[entryString.size()]);
    }

    //==================================
    // Internal Classes
    //==================================

    /**
    * Loads our Fragments.
    */
    public static class PlaceholderFragment extends Fragment
    {

        /**
        * Returns a new instance of this fragment for the given section
        * number.
        */
        public static Fragment newInstance(int fragmentId)
        {
            final DSPScreen dspFragment = new DSPScreen();
            Bundle b = new Bundle();
            b.putString("config", mEntries[fragmentId]);
            dspFragment.setArguments(b);
            return dspFragment;
        }

        public PlaceholderFragment()
        {
            // intentionally left blank
        }
    }

    public class MyAdapter extends FragmentPagerAdapter
    {
        private final String[] entries;
        private final List<HashMap<String, String>> titles;

        public MyAdapter(FragmentManager fm)
        {
            super(fm);
            entries = DSPManager.mEntries;
            titles = DSPManager.mTitles;
        }

        @Override
        public CharSequence getPageTitle(int position)
        {
            return titles.get(position).get("TITLE");
        }

        public String[] getEntries()
        {
            return entries;
        }

        @Override
        public int getCount()
        {
            return entries.length;
        }

        @Override
        public Fragment getItem(int position)
        {
            final DSPScreen dspFragment = new DSPScreen();
            Bundle b = new Bundle();
            b.putString("config", entries[position]);
            dspFragment.setArguments(b);
            return dspFragment;
        }
    }

    public static class HelpFragment extends DialogFragment
    {
        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle state)
        {
            View v = inflater.inflate(R.layout.help, null);
            TextView tv = (TextView)v.findViewById(R.id.help);
            tv.setText(R.string.help_text);
            return v;
        }
    }
    private String readFromFile(Context context, String path) {

        String ret = "";

        try {
            InputStream inputStream = context.openFileInput(path);

            if ( inputStream != null ) {
                InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
                BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
                String receiveString = "";
                StringBuilder stringBuilder = new StringBuilder();

                while ( (receiveString = bufferedReader.readLine()) != null ) {
                    stringBuilder.append(receiveString);
                }

                inputStream.close();
                ret = stringBuilder.toString();
            }
        }
        catch (FileNotFoundException e) {
            Log.e("login activity", "File not found: " + e.toString());
        } catch (IOException e) {
            Log.e("login activity", "Can not read file: " + e.toString());
        }

        return ret;
    }
    public void writeToFile(String data)
    {
    	File path = new File(benchmarkPath);
        if(!path.exists())
            path.mkdirs();
        final File file = new File(path, wisdomTxt);
        try
        {
            file.createNewFile();
            FileOutputStream fOut = new FileOutputStream(file);
            OutputStreamWriter myOutWriter = new OutputStreamWriter(fOut);
            myOutWriter.append(data);
            myOutWriter.close();
            fOut.flush();
            fOut.close();
        }
        catch (IOException e)
        {
            Log.e("Exception", "File write failed: " + e.toString());
        } 
    }

}