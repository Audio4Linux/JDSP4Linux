package james.dsp.service;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.media.audiofx.AudioEffect;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;
import james.dsp.R;
import james.dsp.activity.DSPManager;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import james.dsp.activity.JdspImpResToolbox;
/**
* <p>This calls listen to events that affect DSP function and responds to them.</p>
* <ol>
* <li>new audio session declarations</li>
* <li>headset plug / unplug events</li>
* <li>preference update events.</li>
* </ol>
*
* @Co-founder alankila
*/
public class HeadsetService extends Service
{
	/**
	* Helper class representing the full complement of effects attached to one
	* audio session.
	*
	* @Co-founder alankila
	*/
	Bitmap iconLarge;
	public static int modeEffect;
	public final static UUID EFFECT_TYPE_CUSTOM = UUID.fromString("f98765f4-c321-5de6-9a45-123459495ab2");
	public final static UUID EFFECT_JAMESDSP = UUID.fromString("f27317f4-c984-4de6-9a90-545759495bf2");
	public class JDSPModule
	{
		public AudioEffect JamesDSP;
		public JDSPModule(int sessionId)
		{
			try
			{
				/*
				* AudioEffect constructor is not part of SDK. We use reflection
				* to access it.
				*/
				JamesDSP = AudioEffect.class.getConstructor(UUID.class,
					UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
						EFFECT_TYPE_CUSTOM, EFFECT_JAMESDSP, 0, sessionId);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
			if (DSPManager.devMsgDisplay)
			{
				if (JamesDSP != null)
				{
					AudioEffect.Descriptor dspDescriptor = JamesDSP.getDescriptor();
					if (dspDescriptor.uuid.equals(EFFECT_JAMESDSP))
						Toast.makeText(HeadsetService.this, "Effect name: " + dspDescriptor.name + "\nType id: " + dspDescriptor.type
							+ "\nUnique id: " + dspDescriptor.uuid + "\nImplementor: " + dspDescriptor.implementor, Toast.LENGTH_SHORT).show();
					else
						Toast.makeText(HeadsetService.this, "Effect load failed!\nTry re-enable audio effect in current player!", Toast.LENGTH_SHORT).show();
				}
			}
		}

		public void release()
		{
			JamesDSP.release();
		}

		/**
		* Proxies call to AudioEffect.setParameter(byte[], byte[]) which is
		* available via reflection.
		*
		* @param audioEffect
		* @param parameter
		* @param value
		*/
		private byte[] IntToByte(int[] input)
		{
			int int_index, byte_index;
			int iterations = input.length;
			byte[] buffer = new byte[input.length * 4];
			int_index = byte_index = 0;
			for (; int_index != iterations;)
			{
				buffer[byte_index] = (byte)(input[int_index] & 0x00FF);
				buffer[byte_index + 1] = (byte)((input[int_index] & 0xFF00) >> 8);
				buffer[byte_index + 2] = (byte)((input[int_index] & 0xFF0000) >> 16);
				buffer[byte_index + 3] = (byte)((input[int_index] & 0xFF000000) >> 24);
				++int_index;
				byte_index += 4;
			}
			return buffer;
		}
		private int byteArrayToInt(byte[] encodedValue)
		{
			int value = (encodedValue[3] << 24);
			value |= (encodedValue[2] & 0xFF) << 16;
			value |= (encodedValue[1] & 0xFF) << 8;
			value |= (encodedValue[0] & 0xFF);
			return value;
		}
		private void setParameterIntArray(AudioEffect audioEffect, int parameter, int value[])
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = IntToByte(value);
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private void setParameterFloatArray(AudioEffect audioEffect, int parameter, float value[])
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[value.length * 4];
            	ByteBuffer byteDataBuffer = ByteBuffer.wrap(result);
            	byteDataBuffer.order(ByteOrder.nativeOrder());
            	for (int i = 0; i < value.length; i++)
            		byteDataBuffer.putFloat(value[i]);
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private byte[] concatArrays(byte[]... arrays)
		{
            int len = 0;
            for (byte[] a : arrays)
                len += a.length;
            byte[] b = new byte[len];
            int offs = 0;
            for (byte[] a : arrays)
            {
                System.arraycopy(a, 0, b, offs, a.length);
                offs += a.length;
            }
            return b;
        }
		private void setParameterCharArray(AudioEffect audioEffect, int parameter, String value)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
	            byte[] result = value.getBytes(Charset.forName("US-ASCII"));
                if (result.length < 256)
                {
                    int zeroPad = 256 - result.length;
                    byte[] zeroArray = new byte[zeroPad];
                    result = concatArrays(result, zeroArray);
                    zeroArray = null;
                }
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
                result = null;
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private void setParameterInt(AudioEffect audioEffect, int parameter, int value)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[]
				{
					(byte)(value), (byte)(value >> 8),
					(byte)(value >> 16), (byte)(value >> 24)
				};
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private void setParameterShort(AudioEffect audioEffect, int parameter, short value)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[]
				{
					(byte)(value), (byte)(value >> 8)
				};
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private int getParameter(AudioEffect audioEffect, int parameter)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[4];
				Method getParameter = AudioEffect.class.getMethod("getParameter", byte[].class, byte[].class);
				getParameter.invoke(audioEffect, arguments, result);
				return byteArrayToInt(result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
	}

	public class LocalBinder extends Binder
	{
		public HeadsetService getService()
		{
			return HeadsetService.this;
		}
	}

	private final LocalBinder mBinder = new LocalBinder();

	/**
	* Known audio sessions and their associated audioeffect suites.
	*/
	private final Map<Integer, JDSPModule> mAudioSessions = new HashMap<Integer, JDSPModule>();

	/**
	* Is a wired headset plugged in?
	*/
	public static boolean mUseHeadset = false;

	/**
	* Is bluetooth headset plugged in?
	*/
	public static boolean mUseBluetooth = false;

	private float[] mOverriddenEqualizerLevels;

	private float[] eqLevels = new float[15];
	public static float[] bench_c0 = new float[10];
	public static float[] bench_c1 = new float[10];
	public static int benchNum = 0;
	/**
	* Receive new broadcast intents for adding DSP to session
	*/
	private String oldVDCName = "";
	private String oldImpulseName = "";
	private float oldQuality = 0;
	private float prelimthreshold = 0;
	private float prelimrelease = 0;
	private int olddBGainIR = 0;
	public static JDSPModule JamesDSPGbEf;
	private SharedPreferences preferencesMode;
	public static int dspModuleSamplingRate = 0;
	public static String ddcString = "";
	private final BroadcastReceiver mAudioSessionReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(Context context, Intent intent)
	{
		String action = intent.getAction();
		int sessionId = intent.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION, 0);
		if (sessionId == 0)
			return;
		if (action.equals(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION))
		{
			if (modeEffect == 0)
				return;
			if (!mAudioSessions.containsKey(sessionId)) {
				JDSPModule fxId = new JDSPModule(sessionId);
				if (fxId.JamesDSP == null)
				{
					Log.e(DSPManager.TAG, "Audio session load fail");
					fxId.release();
					fxId = null;
				}
				else
					mAudioSessions.put(sessionId, fxId);
				updateDsp(false, true);
			}
		}
		if (action.equals(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION))
		{
			JDSPModule gone = mAudioSessions.remove(sessionId);
			if (gone != null)
				gone.release();
			gone = null;
		}
	}
	};
	/**
	* Update audio parameters when preferences have been updated.
	*/
	private final BroadcastReceiver mPreferenceUpdateReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(Context context, Intent intent)
	{
		updateDsp(false, true);
	}
	};

	/**
	* This code listens for changes in bluetooth and headset events. It is
	* adapted from google's own MusicFX application, so it's presumably the
	* most correct design there is for this problem.
	*/
	private final BroadcastReceiver mRoutingReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
	{
		final String action = intent.getAction();
		final boolean prevUseHeadset = mUseHeadset;
		if (action.equals(AudioManager.ACTION_HEADSET_PLUG))
			mUseHeadset = intent.getIntExtra("state", 0) == 1;
		 else
             if (Build.VERSION.SDK_INT >= 16 && action.equals("android.intent.action.ANALOG_AUDIO_DOCK_PLUG"))
            	 mUseHeadset = intent.getIntExtra("state", 0) == 1;
		if (prevUseHeadset != mUseHeadset)
			updateDsp(true, false);
	}
	};

	private final BroadcastReceiver mBtReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
	{
		final String action = intent.getAction();
		if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED))
		{
			int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE, BluetoothProfile.STATE_CONNECTED);
			if (state == BluetoothProfile.STATE_CONNECTED && !mUseBluetooth)
			{
				mUseBluetooth = true;
				updateDsp(true, false);
			}
			else if (mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, false);
			}
		}
		else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED))
		{
			String stateExtra = BluetoothAdapter.EXTRA_STATE;
			int state = intent.getIntExtra(stateExtra, -1);
			if (state == BluetoothAdapter.STATE_OFF && mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, false);
			}
		}
	}
	};
	private void foregroundPersistent(String mFXType)
	{
		int mIconIDSmall = getResources().getIdentifier("ic_stat_icon", "drawable", getApplicationInfo().packageName);
		Intent notificationIntent = new Intent(this, DSPManager.class);
		PendingIntent contentItent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		Notification statusNotify = new Notification.Builder(this)
			.setPriority(Notification.PRIORITY_MIN)
			.setAutoCancel(false)
			.setOngoing(true)
			.setDefaults(0)
			.setWhen(System.currentTimeMillis())
			.setSmallIcon(mIconIDSmall)
			.setLargeIcon(iconLarge)
			.setTicker("JamesDSP " + mFXType)
			.setContentTitle("JamesDSP")
			.setContentText(mFXType)
			.setContentIntent(contentItent)
			.build();
		startForeground(1, statusNotify);
	}

class StartUpOptimiserThread implements Runnable {
	int times;
	int tarSmpRate;
	StartUpOptimiserThread(int entriesGen, int fs) {
		if (entriesGen > 10)
			entriesGen = 10;
	       times = entriesGen;
	       tarSmpRate = fs;
	   }
	   public void run() {
		   double[] c0 = new double[10];
		   double[] c1 = new double[10];
		   benchNum = JdspImpResToolbox.FFTConvolutionBenchmark(times, tarSmpRate, c0, c1);
			for (int i = 0 ; i < benchNum; i++)
			{
				bench_c0[i] = (float)c0[i];
				bench_c1[i] = (float)c1[i];
			}
/*			Log.i(DSPManager.TAG, "(Int)c0: " + Arrays.toString(bench_c0));
			Log.i(DSPManager.TAG, "(Int)c1: " + Arrays.toString(bench_c1));*/
			updateDsp(true, false);
	   }
	}
	@Override
	public void onCreate()
	{
		super.onCreate();
		IntentFilter audioFilter = new IntentFilter();
		audioFilter.addAction(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION);
		audioFilter.addAction(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION);
		registerReceiver(mAudioSessionReceiver, audioFilter);
		final IntentFilter intentFilter = new IntentFilter(AudioManager.ACTION_HEADSET_PLUG);
        if (Build.VERSION.SDK_INT >= 16)
            audioFilter.addAction("android.intent.action.ANALOG_AUDIO_DOCK_PLUG");
		registerReceiver(mRoutingReceiver, intentFilter);
		registerReceiver(mPreferenceUpdateReceiver, new IntentFilter(DSPManager.ACTION_UPDATE_PREFERENCES));
		final IntentFilter btFilter = new IntentFilter();
		btFilter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
		btFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
		registerReceiver(mBtReceiver, btFilter);AudioManager mAudioManager = (AudioManager) getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
        if (mAudioManager != null)
        {
            mUseBluetooth = mAudioManager.isBluetoothA2dpOn();
            if (mUseBluetooth)
            {
                Log.i(DSPManager.TAG, "Bluetooth mode");
                mUseHeadset = false;
            }
            else
            {
                mUseHeadset = mAudioManager.isWiredHeadsetOn();
                if (mUseHeadset)
                    Log.i(DSPManager.TAG, "Headset mode");
                else
                    Log.i(DSPManager.TAG, "Speaker mode");
            }
        }
		iconLarge = BitmapFactory.decodeResource(getResources(), R.drawable.icon);
		preferencesMode = getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + "settings", 0);
		if (!preferencesMode.contains("dsp.app.modeEffect"))
			preferencesMode.edit().putInt("dsp.app.modeEffect", 0).commit();
		modeEffect = preferencesMode.getInt("dsp.app.modeEffect", 0);
		if (JamesDSPGbEf != null)
		{
			JamesDSPGbEf.release();
			JamesDSPGbEf = null;
		}
		if (modeEffect == 0)
		{
			if (JamesDSPGbEf == null)
				JamesDSPGbEf = new JDSPModule(0);
			if (JamesDSPGbEf.JamesDSP == null)
			{
				Toast.makeText(HeadsetService.this, "Library load failed(Global effect)", Toast.LENGTH_SHORT).show();
				JamesDSPGbEf.release();
				JamesDSPGbEf = null;
			}
		}
		updateDsp(true, false);
		if (bench_c0[0] == 0 || bench_c1[1] == 0)
		{
	        Runnable runner = new StartUpOptimiserThread(10, 48000);
	        new Thread(runner).start();
		}
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		unregisterReceiver(mAudioSessionReceiver);
		unregisterReceiver(mRoutingReceiver);
		unregisterReceiver(mPreferenceUpdateReceiver);
		stopForeground(true);
		if (iconLarge != null)
		{
			iconLarge.recycle();
			iconLarge = null;
		}
		mAudioSessions.clear();
		if (JamesDSPGbEf != null)
			JamesDSPGbEf.release();
		JamesDSPGbEf = null;
	}
	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
	{
		modeEffect = preferencesMode.getInt("dsp.app.modeEffect", 0);
		if (modeEffect == 0)
		{
			if (JamesDSPGbEf == null) {
				JamesDSPGbEf = new JDSPModule(0);
				if (JamesDSPGbEf.JamesDSP == null) {
					Log.e(DSPManager.TAG, "Global audio session load fail, reload it now!");
					JamesDSPGbEf.release();
					JamesDSPGbEf = null;
					return super.onStartCommand(intent, flags, startId);
				}
				updateDsp(false, true);
				return super.onStartCommand(intent, flags, startId);
			}
			if (JamesDSPGbEf.JamesDSP == null) {
				Log.e(DSPManager.TAG, "Global audio session load fail, reload it now!");
				JamesDSPGbEf.release();
				JamesDSPGbEf = new JDSPModule(0);
				if (JamesDSPGbEf.JamesDSP == null) {
					Log.e(DSPManager.TAG, "Global audio session load fail, reload it now!");
					JamesDSPGbEf.release();
					JamesDSPGbEf = null;
					return super.onStartCommand(intent, flags, startId);
				}
				return super.onStartCommand(intent, flags, startId);
			}
			Log.i(DSPManager.TAG, "Global audio session created!");
			updateDsp(false, true);
		}
		return super.onStartCommand(intent, flags, startId);
	}
	@Override
	public IBinder onBind(Intent intent)
	{
		return mBinder;
	}

	/**
	* Gain temporary control over the global equalizer.
	* Used by DSPManager when testing a new equalizer setting.
	*
	* @param levels
	*/
	public void setEqualizerLevels(float[] levels)
	{
		mOverriddenEqualizerLevels = levels;
		updateDsp(false, false);
	}

	public static String getAudioOutputRouting()
	{
		if (mUseBluetooth)
			return "bluetooth";
		if (mUseHeadset)
			return "headset";
		return "speaker";
	}

	/**
	* Push new configuration to audio stack.
	*/
	protected void updateDsp(boolean notify, boolean updateConvolver)
	{
		modeEffect = preferencesMode.getInt("dsp.app.modeEffect", 0);
		final String mode = getAudioOutputRouting();
		SharedPreferences preferences = getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + mode, 0);
		if (notify)
		{
			String pid = "";
			if (JamesDSPGbEf != null)
				pid = " PID:" + JamesDSPGbEf.getParameter(JamesDSPGbEf.JamesDSP, 20002);
			if (mode == "bluetooth")
				foregroundPersistent(getString(R.string.bluetooth_title) + pid);
			else if (mode == "headset")
				foregroundPersistent(getString(R.string.headset_title) + pid);
			else
				foregroundPersistent(getString(R.string.speaker_title) + pid);
			Intent intent = new Intent("dsp.activity.updatePage");
			sendBroadcast(intent);
		}
		if (modeEffect == 0)
		{
			try
			{
				updateDsp(preferences, JamesDSPGbEf, updateConvolver, 0);
			}
			catch (Exception e)
			{
			}
		}
		else
		{
			for (Integer sessionId : new ArrayList<Integer>(mAudioSessions.keySet()))
			{
				try
				{
					updateDsp(preferences, mAudioSessions.get(sessionId), updateConvolver, sessionId);
				}
				catch (Exception e)
				{
					mAudioSessions.remove(sessionId);
				}
			}
		}
	}

	private void updateDsp(SharedPreferences preferences, JDSPModule session, boolean updateMajor, int sessionId)
	{
		boolean masterSwitch = preferences.getBoolean("dsp.masterswitch.enable", false);
		session.JamesDSP.setEnabled(masterSwitch); // Master switch
		if (masterSwitch)
		{
			int compressorEnabled = preferences.getBoolean("dsp.compression.enable", false) ? 1 : 0;
			int bassBoostEnabled = preferences.getBoolean("dsp.bass.enable", false) ? 1 : 0;
			int equalizerEnabled = preferences.getBoolean("dsp.tone.enable", false) ? 1 : 0;
			int reverbEnabled = preferences.getBoolean("dsp.headphone.enable", false) ? 1 : 0;
			int stereoWideEnabled = preferences.getBoolean("dsp.stereowide.enable", false) ? 1 : 0;
			int bs2bEnabled = preferences.getBoolean("dsp.bs2b.enable", false) ? 1 : 0;
			int convolverEnabled = preferences.getBoolean("dsp.convolver.enable", false) ? 1 : 0;
			int analogModelEnabled = preferences.getBoolean("dsp.analogmodelling.enable", false) ? 1 : 0;
			int viperddcEnabled = preferences.getBoolean("dsp.ddc.enable", false) ? 1 : 0;
//			int wavechild670Enabled = preferences.getBoolean("dsp.wavechild670.enable", false) ? 1 : 0;
			dspModuleSamplingRate = session.getParameter(session.JamesDSP, 20000);
			if (dspModuleSamplingRate == 0)
			{
				if (JamesDSPGbEf.getParameter(JamesDSPGbEf.JamesDSP, 20002) == 0)
				{
					Toast.makeText(HeadsetService.this, R.string.dspneedreboot, Toast.LENGTH_LONG).show();
					return;
				}
				Toast.makeText(HeadsetService.this, R.string.dspcrashed, Toast.LENGTH_LONG).show();
				return;
			}
			int convolutionBenchmarkDataReady = session.getParameter(session.JamesDSP, 20003);
			if (convolutionBenchmarkDataReady == 0 && bench_c0[0] > 0.0f && bench_c1[0] > 0.0f)
			{
				session.setParameterFloatArray(session.JamesDSP, 1997, bench_c0);
				session.setParameterFloatArray(session.JamesDSP, 1998, bench_c1);
			}
			float limthreshold = Float.valueOf(preferences.getString("dsp.masterswitch.limthreshold", "-0.1"));
			float limrelease = Float.valueOf(preferences.getString("dsp.masterswitch.limrelease", "60"));
			if (prelimthreshold != limthreshold || prelimrelease != limrelease)
				session.setParameterFloatArray(session.JamesDSP, 1500, new float[]{ limthreshold, limrelease });
			prelimthreshold = limthreshold;
			prelimrelease = limrelease;
			if (compressorEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 100, Short.valueOf(preferences.getString("dsp.compression.pregain", "12")));
				session.setParameterShort(session.JamesDSP, 101, (short)Math.abs(Short.valueOf(preferences.getString("dsp.compression.threshold", "-60"))));
				session.setParameterShort(session.JamesDSP, 102, Short.valueOf(preferences.getString("dsp.compression.knee", "30")));
				session.setParameterShort(session.JamesDSP, 103, Short.valueOf(preferences.getString("dsp.compression.ratio", "12")));
				session.setParameterShort(session.JamesDSP, 104, (short)(Float.valueOf((preferences.getString("dsp.compression.attack", "0.01")))*1000));
				session.setParameterShort(session.JamesDSP, 105, (short)(Float.valueOf((preferences.getString("dsp.compression.release", "0.01")))*1000));
			}
			session.setParameterShort(session.JamesDSP, 1200, (short)compressorEnabled); // Compressor switch
			session.setParameterShort(session.JamesDSP, 1201, (short)bassBoostEnabled); // Bass boost switch
			if (bassBoostEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 112, Short.valueOf(preferences.getString("dsp.bass.mode", "80")));
				short filtertype = Short.valueOf(preferences.getString("dsp.bass.filtertype", "0"));
				session.setParameterShort(session.JamesDSP, 113, filtertype);
				short freq = Short.valueOf(preferences.getString("dsp.bass.freq", "55"));
				session.setParameterShort(session.JamesDSP, 114, freq);
				if(DSPManager.devMsgDisplay)
				if(filtertype == 0 && freq < 90)
						Toast.makeText(HeadsetService.this, R.string.preferredlpf, Toast.LENGTH_SHORT).show();
			}
			session.setParameterShort(session.JamesDSP, 151, Short.valueOf(preferences.getString("dsp.tone.filtertype", "0")));
			session.setParameterShort(session.JamesDSP, 1202, (short)equalizerEnabled); // Equalizer switch
			if (equalizerEnabled == 1)
			{
				/* Equalizer state is in a single string preference with all values separated by ; */
				if (mOverriddenEqualizerLevels != null)
				{
					for (short i = 0; i < mOverriddenEqualizerLevels.length; i++)
						eqLevels[i] = mOverriddenEqualizerLevels[i];
				}
				else
				{
					String[] levels = preferences.getString("dsp.tone.eq.custom", "0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0").split(";");
					for (short i = 0; i < levels.length; i++)
						eqLevels[i] = Float.valueOf(levels[i]);
				}
				session.setParameterFloatArray(session.JamesDSP, 115, eqLevels);
			}
			if (reverbEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 128, Short.valueOf(preferences.getString("dsp.headphone.preset", "0")));
			session.setParameterShort(session.JamesDSP, 1203, (short)reverbEnabled); // Reverb switch
			if (stereoWideEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 137, Short.valueOf(preferences.getString("dsp.stereowide.mode", "0")));
			session.setParameterShort(session.JamesDSP, 1204, (short)stereoWideEnabled); // Stereo widener switch
			session.setParameterShort(session.JamesDSP, 1208, (short)bs2bEnabled); // BS2B switch
			if (bs2bEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 188, Short.valueOf(preferences.getString("dsp.bs2b.mode", "0")));
			if (analogModelEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 150, (short) (Float.valueOf(preferences.getString("dsp.analogmodelling.tubedrive", "2"))*1000));
			session.setParameterShort(session.JamesDSP, 1206, (short)analogModelEnabled); // Analog modelling switch
			if (viperddcEnabled == 1 && updateMajor)
			{
				String ddcFilePath = preferences.getString("dsp.ddc.files", "");
				if (!oldVDCName.equals(ddcFilePath))
				{
					oldVDCName = ddcFilePath;
					StringBuilder contentBuilder = new StringBuilder();
				    try (BufferedReader br = new BufferedReader(new FileReader(ddcFilePath)))
				    {
				        String sCurrentLine;
				        while ((sCurrentLine = br.readLine()) != null)
				            contentBuilder.append(sCurrentLine).append("\n");
				    }
				    catch (IOException e)
				    {
				        e.printStackTrace();
				    }
					int arraySize2Send = 256;
				    ddcString = contentBuilder.toString();
				    if(ddcString != null && !ddcString.isEmpty())
				    {
						int stringLength = ddcString.length();
						int numTime2Send = (int)Math.ceil((double)stringLength / arraySize2Send); // Number of times that have to send
						session.setParameterIntArray(session.JamesDSP, 8888, new int[]{ numTime2Send, arraySize2Send }); // Send buffer info for module to allocate memory
						for (int i = 0; i < numTime2Send; i++)
						{
							session.setParameterShort(session.JamesDSP, 10005, (short)i); // Increment sliced buffer
							session.setParameterCharArray(session.JamesDSP, 12001, ddcString.substring(arraySize2Send * i, Math.min(arraySize2Send * i + arraySize2Send, stringLength))); // Commit buffer
						}
						session.setParameterShort(session.JamesDSP, 10009, (short)1); // Notify send array completed and generate filter in native side
				    }
				}
			}
			else
				oldVDCName = "";
			session.setParameterShort(session.JamesDSP, 1212, (short)viperddcEnabled); // VDC switch
			if (convolverEnabled == 1 && updateMajor)
			{
				String mConvIRFilePath = preferences.getString("dsp.convolver.files", "");
				float quality = Float.valueOf(preferences.getString("dsp.convolver.quality", "1"));
				int dBGainIR = (int)(Math.ceil(Float.valueOf(preferences.getString("dsp.convolver.gain", "0.0")) * 262144.0f));
//				int requiredRefresh = session.getParameter(session.JamesDSP, 20001);
				if (modeEffect == 0)
				{
					if(oldImpulseName.equals(mConvIRFilePath) && oldQuality == quality && olddBGainIR == dBGainIR)
						return;
				}
				oldImpulseName = mConvIRFilePath;
				oldQuality = quality;
				olddBGainIR = dBGainIR;
				session.setParameterShort(session.JamesDSP, 1205, (short)0);
				String mConvIRFileName = mConvIRFilePath.replace(DSPManager.impulseResponsePath, "");
				int[] impinfo = JdspImpResToolbox.GetLoadImpulseResponseInfo(mConvIRFilePath);
				if (impinfo == null)
					Toast.makeText(HeadsetService.this, R.string.impfilefault, Toast.LENGTH_SHORT).show();
				else
				{
					if (impinfo[2] != dspModuleSamplingRate)
						Toast.makeText(HeadsetService.this, getString(R.string.unmatchedsamplerate, mConvIRFilePath, impinfo[2], dspModuleSamplingRate), Toast.LENGTH_SHORT).show();
					float[] impulseResponse = JdspImpResToolbox.ReadImpulseResponseToFloat(dspModuleSamplingRate);
					if (impulseResponse == null)
						Toast.makeText(HeadsetService.this, R.string.memoryallocatefail, Toast.LENGTH_SHORT).show();
					else
					{
						int arraySize2Send = 4096;
						impinfo[1] = impulseResponse.length / impinfo[0];
						int impulseCutted = impulseResponse.length;
						impulseCutted = (int)(impulseCutted * quality);
						float[] sendArray = new float[arraySize2Send];
						int numTime2Send = (int)Math.ceil((double)impulseCutted / arraySize2Send); // Send number of times that have to send
						int[] sendBufInfo = new int[] {impulseCutted, impinfo[0], (int)(Math.ceil(Float.valueOf(preferences.getString("dsp.convolver.gain", "0.0")) * 262144.0f)), numTime2Send};
						session.setParameterIntArray(session.JamesDSP, 9999, sendBufInfo); // Send buffer info for module to allocate memory
						float[] finalArray = new float[numTime2Send*arraySize2Send]; // Fill final array with zero padding
						System.arraycopy(impulseResponse, 0, finalArray, 0, impulseCutted);
						for (int i = 0; i < numTime2Send; i++)
						{
							System.arraycopy(finalArray, arraySize2Send * i, sendArray, 0, arraySize2Send);
							session.setParameterShort(session.JamesDSP, 10003, (short)i); // Increment sliced buffer
							session.setParameterFloatArray(session.JamesDSP, 12000, sendArray); // Commit buffer
						}
						session.setParameterShort(session.JamesDSP, 10004, (short)1); // Notify send array completed and resize array in native side
						if (DSPManager.devMsgDisplay)
						{
							int convolutionStrategy = session.getParameter(session.JamesDSP, 20004);
							if (impinfo[0] == 1)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, impinfo[2], getString(R.string.mono_conv), impinfo[1], (int)impulseCutted, convolutionStrategy), Toast.LENGTH_SHORT).show();
							else if (impinfo[0] == 2)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, impinfo[2], getString(R.string.stereo_conv), impinfo[1], (int)impulseCutted / 2, convolutionStrategy), Toast.LENGTH_SHORT).show();
							else if (impinfo[0] == 4)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, impinfo[2], getString(R.string.fullstereo_conv), impinfo[1], (int)impulseCutted / 4, convolutionStrategy), Toast.LENGTH_SHORT).show();
						}
					}
				}
			}
			else
			{
				oldImpulseName = "";
				oldQuality = 0;
			}
			session.setParameterShort(session.JamesDSP, 1205, (short)convolverEnabled); // Convolver switch
		}
	}
}