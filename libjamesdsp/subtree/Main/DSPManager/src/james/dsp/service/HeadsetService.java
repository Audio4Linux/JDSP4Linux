package james.dsp.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
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
import android.graphics.Color;
import android.media.AudioManager;
import android.media.audiofx.AudioEffect;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;
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
		private int getParameterInt(AudioEffect audioEffect, int parameter)
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

	private double[] EQ_Input_Levels;
	private double[] EQ_Output_Levels;
	private float[] eqLevels = new float[30];
	private double[] COMP_Input_Levels;
	private double[] COMP_Output_Levels;
	private float[] compLevels = new float[14];
	/**
	* Receive new broadcast intents for adding DSP to session
	*/
	private float prelimthreshold = 0;
	private float prelimrelease = 0;
	private float prepostgain = 0;
	public static JDSPModule JamesDSPGbEf;
	private SharedPreferences preferencesMode;
	public static int dspModuleSamplingRate = 0;
	
	final static public float[] mergeFloatArray(final float[] ...arrays)
	{
	    int size = 0;
	    for (float[] a: arrays)
	        size += a.length;
	    float[] res = new float[size];
        int destPos = 0;
        for (int i = 0; i < arrays.length; i++ )
        {
            if (i > 0)
            	destPos += arrays[i-1].length;
            int length = arrays[i].length;
            System.arraycopy(arrays[i], 0, res, destPos, length);
        }
        return res;
	}
	final static public int HashString(final String str)
	{
		byte[] btAry = str.getBytes();
		int crc = 0xFFFFFFFF;
		for (int i = 0; i < btAry.length; i++)
		{
			crc = crc ^ btAry[i];
			for (int j = 7; j >= 0; j--)
			{
				int mask = -(crc & 1);
				crc = (crc >> 1) ^ (0xEDB88320 & mask);
			}
		}
		return ~crc;
	}
	
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
			updateDsp(true, true);
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
				updateDsp(true, true);
			}
			else if (mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, true);
			}
		}
		else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED))
		{
			String stateExtra = BluetoothAdapter.EXTRA_STATE;
			int state = intent.getIntExtra(stateExtra, -1);
			if (state == BluetoothAdapter.STATE_OFF && mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, true);
			}
		}
	}
	};
	private void foregroundPersistent(String mFXType)
	{
		String NOTIFICATION_CHANNEL_ID = "james.dsp.service";
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
		    String channelName = "JDSP compat O";
		    NotificationChannel chan = new NotificationChannel(NOTIFICATION_CHANNEL_ID, channelName, NotificationManager.IMPORTANCE_NONE);
		    chan.setLightColor(Color.BLUE);
		    chan.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
		    NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		    assert manager != null;
		    manager.createNotificationChannel(chan);
		}
		int mIconIDSmall = getResources().getIdentifier("ic_stat_icon", "drawable", getApplicationInfo().packageName);
		Intent notificationIntent = new Intent(this, DSPManager.class);
		PendingIntent contentItent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			Notification statusNotify = new Notification.Builder(this, NOTIFICATION_CHANNEL_ID)
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
				startForeground(2, statusNotify);
		}
		else
		{
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
		updateDsp(true, true);
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
	public void setEQLevels(double[] in, double[] out)
	{
		EQ_Input_Levels = in;
		EQ_Output_Levels = out;
		updateDsp(false, false);
	}
	public void setCompLevels(double[] in, double[] out)
	{
		COMP_Input_Levels = in;
		COMP_Output_Levels = out;
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
				pid = " PID:" + JamesDSPGbEf.getParameterInt(JamesDSPGbEf.JamesDSP, 20002);
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
		Log.e("HeadsetService", "sessionId = " + sessionId);
		boolean masterSwitch = preferences.getBoolean("dsp.masterswitch.enable", false);
		session.JamesDSP.setEnabled(masterSwitch); // Master switch
		if (masterSwitch)
		{
			int compressorEnabled = preferences.getBoolean("dsp.compression.enable", false) ? 1 : 0;
			int bassBoostEnabled = preferences.getBoolean("dsp.bass.enable", false) ? 1 : 0;
			int equalizerEnabled = preferences.getBoolean("dsp.tone.enable", false) ? 1 : 0;
			int stringEqEnabled = preferences.getBoolean("dsp.streq.enable", false) ? 1 : 0;
			int reverbEnabled = preferences.getBoolean("dsp.headphone.enable", false) ? 1 : 0;
			int stereoWideEnabled = preferences.getBoolean("dsp.stereowide.enable", false) ? 1 : 0;
			int bs2bEnabled = preferences.getBoolean("dsp.bs2b.enable", false) ? 1 : 0;
			int convolverEnabled = preferences.getBoolean("dsp.convolver.enable", false) ? 1 : 0;
			int analogModelEnabled = preferences.getBoolean("dsp.analogmodelling.enable", false) ? 1 : 0;
			int viperddcEnabled = preferences.getBoolean("dsp.ddc.enable", false) ? 1 : 0;
			int liveProgEnabled = preferences.getBoolean("dsp.liveprog.enable", false) ? 1 : 0;
			int numberOfParameterCommitted = session.getParameterInt(session.JamesDSP, 19998);
//			Log.i(DSPManager.TAG, "Commited: " + numberOfParameterCommitted);
			int dspBufferLen = session.getParameterInt(session.JamesDSP, 19999);
			int dspAllocatedBlockLen = session.getParameterInt(session.JamesDSP, 20000);
			dspModuleSamplingRate = session.getParameterInt(session.JamesDSP, 20001);
			if (dspModuleSamplingRate == 0)
			{
				if (session.getParameterInt(session.JamesDSP, 20002) == 0)
				{
					Toast.makeText(HeadsetService.this, R.string.dspneedreboot, Toast.LENGTH_LONG).show();
					Log.e("HeadsetService", "Get PID failed from engine");
				}
				Toast.makeText(HeadsetService.this, R.string.dspcrashed, Toast.LENGTH_LONG).show();
				Log.e("HeadsetService", "Get zero sample rate from engine? Resurrecting service!");
				if (modeEffect == 0)
				{
					Log.e(DSPManager.TAG, "Global audio session have been killed, reload it now!");
					JamesDSPGbEf.release();
					JamesDSPGbEf = new JDSPModule(0);
					JamesDSPGbEf.JamesDSP.setEnabled(masterSwitch); // Master switch
				}
				else
				{
					Log.e(DSPManager.TAG, "Audio session " + sessionId + " have been killed, reload it now!");
					session.release();
					session = new JDSPModule(sessionId);
					session.JamesDSP.setEnabled(masterSwitch); // Master switch
				}
			}
			float limthreshold = Float.valueOf(preferences.getString("dsp.masterswitch.limthreshold", "-0.1"));
			float limrelease = Float.valueOf(preferences.getString("dsp.masterswitch.limrelease", "60"));
			float postgain = Float.valueOf(preferences.getString("dsp.masterswitch.postgain", "0"));
			if (prelimthreshold != limthreshold || prelimrelease != limrelease || prepostgain != postgain)
				session.setParameterFloatArray(session.JamesDSP, 1500, new float[]{ limthreshold, limrelease, postgain });
			prelimthreshold = limthreshold;
			prelimrelease = limrelease;
			prepostgain = postgain;
			if (compressorEnabled == 1)
			{
				/* Compander state is in a single string preference with all values separated by ; */
				if (COMP_Input_Levels != null && COMP_Output_Levels != null)
				{
					for (short i = 0; i < COMP_Output_Levels.length; i++)
					{
						compLevels[i] = (float)COMP_Input_Levels[i];
						compLevels[i + 7] = (float)COMP_Output_Levels[i];
					}
				}
				else
				{
					String[] levels = preferences.getString("dsp.compression.eq.custom", "95.0;200.0;400.0;800.0;1600.0;3400.0;7500.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0").split(";");
					for (short i = 0; i < levels.length; i++)
						compLevels[i] = Float.valueOf(levels[i]);
				}
				float timeConstant = Float.valueOf(preferences.getString("dsp.compression.timeconstant", "0.22"));
				float granularity = Float.valueOf(preferences.getString("dsp.compression.granularity", "4"));
				float tfresolution = Float.valueOf(preferences.getString("dsp.compression.tfresolution", "0"));
				float compConfig[] = new float[]{ timeConstant, granularity, tfresolution };
				float sendAry[] = mergeFloatArray(compConfig, compLevels);
				//Log.i(DSPManager.TAG, "Compander: " + Arrays.toString(compLevels));
				session.setParameterFloatArray(session.JamesDSP, 115, sendAry);
			}
			session.setParameterShort(session.JamesDSP, 1200, (short)compressorEnabled); // Compressor switch
			if (bassBoostEnabled == 1)
			{
				short maxg = Short.valueOf(preferences.getString("dsp.bass.maxgain", "5"));
				session.setParameterShort(session.JamesDSP, 112, maxg);
			}
			session.setParameterShort(session.JamesDSP, 1201, (short)bassBoostEnabled); // Bass boost switch
			if (equalizerEnabled == 1)
			{
				/* Equalizer state is in a single string preference with all values separated by ; */
				if (EQ_Input_Levels != null && EQ_Output_Levels != null)
				{
					for (short i = 0; i < EQ_Output_Levels.length; i++)
					{
						eqLevels[i] = (float)EQ_Input_Levels[i];
						eqLevels[i + 15] = (float)EQ_Output_Levels[i];
					}
				}
				else
				{
					String[] levels = preferences.getString("dsp.tone.eq.custom", "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0").split(";");
					for (short i = 0; i < levels.length; i++)
						eqLevels[i] = Float.valueOf(levels[i]);
				}
				short filtertype = Short.valueOf(preferences.getString("dsp.tone.filtertype", "3"));
				float interpolationMode = -1.0f;
				if (Short.valueOf(preferences.getString("dsp.tone.interpolation", "0")) == 1)
					interpolationMode = 1.0f;
				float ftype[] = new float[]{ (float)filtertype, interpolationMode };
				float sendAry[] = mergeFloatArray(ftype, eqLevels);
				//Log.i(DSPManager.TAG, "Equalizer: " + Arrays.toString(eqLevels));
				session.setParameterFloatArray(session.JamesDSP, 116, sendAry);
			}
			session.setParameterShort(session.JamesDSP, 1202, (short)equalizerEnabled); // Equalizer switch
			if (stringEqEnabled == 1 && updateMajor)
			{
				String eqText = preferences.getString("dsp.streq.stringp", "GraphicEQ: 0.0 0.0; ");
				int previousHash = session.getParameterInt(session.JamesDSP, 30000);
		    	int hashID = HashString(eqText);
		    	Log.e(DSPManager.TAG, "ArbEq hash before: " + previousHash + ", current: " + hashID);
		    	if (previousHash != hashID)
		    	{
					int arraySize2Send = 256;
					int stringLength = eqText.length();
					int numTime2Send = (int)Math.ceil((double)stringLength / arraySize2Send); // Number of times that have to send
					session.setParameterIntArray(session.JamesDSP, 8888, new int[]{ numTime2Send, arraySize2Send }); // Send buffer info for module to allocate memory
					for (int i = 0; i < numTime2Send; i++)
						session.setParameterCharArray(session.JamesDSP, 12001, eqText.substring(arraySize2Send * i, Math.min(arraySize2Send * i + arraySize2Send, stringLength))); // Commit buffer
					session.setParameterShort(session.JamesDSP, 10006, (short)1); // Notify send array completed and generate filter in native side
					session.setParameterInt(session.JamesDSP, 25000, hashID); // Commit hashID to engine
		    	}
			}
			session.setParameterShort(session.JamesDSP, 1210, (short)stringEqEnabled); // String equalizer switch
			if (reverbEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 128, Short.valueOf(preferences.getString("dsp.headphone.preset", "0")));
			session.setParameterShort(session.JamesDSP, 1203, (short)reverbEnabled); // Reverb switch
			if (stereoWideEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 137, Short.valueOf(preferences.getString("dsp.stereowide.mode", "60")));
			session.setParameterShort(session.JamesDSP, 1204, (short)stereoWideEnabled); // Stereo widener switch
			if (bs2bEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 188, Short.valueOf(preferences.getString("dsp.bs2b.mode", "0")));
			session.setParameterShort(session.JamesDSP, 1208, (short)bs2bEnabled); // BS2B switch
			if (analogModelEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 150, (short) (Float.valueOf(preferences.getString("dsp.analogmodelling.tubedrive", "2"))*1000));
			session.setParameterShort(session.JamesDSP, 1206, (short)analogModelEnabled); // Analog modelling switch
			if (viperddcEnabled == 1 && updateMajor)
			{
				String ddcFilePath = preferences.getString("dsp.ddc.files", "");
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
				String ddcString = contentBuilder.toString();
			    if(ddcString != null && !ddcString.isEmpty())
			    {
					int previousHash = session.getParameterInt(session.JamesDSP, 30001);
			    	int hashID = HashString(ddcString);
			    	Log.e(DSPManager.TAG, "DDC hash before: " + previousHash + ", current: " + hashID);
			    	if (previousHash != hashID)
			    	{
						int stringLength = ddcString.length();
						int numTime2Send = (int)Math.ceil((double)stringLength / arraySize2Send); // Number of times that have to send
						session.setParameterIntArray(session.JamesDSP, 8888, new int[]{ numTime2Send, arraySize2Send }); // Send buffer info for module to allocate memory
						for (int i = 0; i < numTime2Send; i++)
							session.setParameterCharArray(session.JamesDSP, 12001, ddcString.substring(arraySize2Send * i, Math.min(arraySize2Send * i + arraySize2Send, stringLength))); // Commit buffer
						session.setParameterShort(session.JamesDSP, 10009, (short)1); // Notify send array completed and generate filter in native side
						session.setParameterInt(session.JamesDSP, 25001, hashID); // Commit hashID to engine
			    	}
			    }
			}
			session.setParameterShort(session.JamesDSP, 1212, (short)viperddcEnabled); // VDC switch
			if (liveProgEnabled == 1 && updateMajor)
			{
				String eelFilePath = preferences.getString("dsp.liveprog.files", "");
				StringBuilder contentBuilder = new StringBuilder();
			    try (BufferedReader br = new BufferedReader(new FileReader(eelFilePath)))
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
				String eelProgString = contentBuilder.toString();
			    if(eelProgString != null && !eelProgString.isEmpty())
			    {
					int previousHash = session.getParameterInt(session.JamesDSP, 30002);
			    	int hashID = HashString(eelProgString);
			    	Log.e(DSPManager.TAG, "LiveProg hash before: " + previousHash + ", current: " + hashID);
			    	if (previousHash != hashID)
			    	{
						int stringLength = eelProgString.length();
						int numTime2Send = (int)Math.ceil((double)stringLength / arraySize2Send); // Number of times that have to send
						session.setParameterIntArray(session.JamesDSP, 8888, new int[]{ numTime2Send, arraySize2Send }); // Send buffer info for module to allocate memory
						for (int i = 0; i < numTime2Send; i++)
							session.setParameterCharArray(session.JamesDSP, 12001, eelProgString.substring(arraySize2Send * i, Math.min(arraySize2Send * i + arraySize2Send, stringLength))); // Commit buffer
						session.setParameterShort(session.JamesDSP, 10010, (short)1); // Notify send array completed and generate filter in native side
						session.setParameterInt(session.JamesDSP, 25002, hashID); // Commit hashID to engine
			    	}
			    }
			}
			session.setParameterShort(session.JamesDSP, 1213, (short)liveProgEnabled); // LiveProg switch
			if (convolverEnabled == 1 && updateMajor)
			{
				String mConvIRFilePath = preferences.getString("dsp.convolver.files", "");
				int convMode = Integer.valueOf(preferences.getString("dsp.convolver.mode", "0"));
				String[] advConv = preferences.getString("dsp.convolver.advimp", "-80;-100;23;12;17;28").split(";");
				int[] advSetting = new int[6];
				advSetting[1] = advSetting[0] = -100;
				if (advConv.length == 6)
				{
					for (int i = 0; i < advConv.length; i++)
						advSetting[i] = Integer.valueOf(advConv[i]);
					//Log.e(DSPManager.TAG, "Advance settings: " + Arrays.toString(advSetting));// Debug
				}
				String mConvIRFileName = mConvIRFilePath.replace(DSPManager.impulseResponsePath, "");
				int[] impinfo = new int[3];
				//Log.e(DSPManager.TAG, "Conv mode: " + convMode);// Debug
				float[] impulseResponse = JdspImpResToolbox.ReadImpulseResponseToFloat(mConvIRFilePath, dspModuleSamplingRate, impinfo, convMode, advSetting);
				int previousHash = session.getParameterInt(session.JamesDSP, 30003);
		    	Log.e(DSPManager.TAG, "Convolver hash before: " + previousHash + ", current: " + impinfo[2]);
		    	if (previousHash != impinfo[2])
		    	{
					session.setParameterShort(session.JamesDSP, 1205, (short)0);
					//Log.e(DSPManager.TAG, "Channels: " + impinfo[0] + ", frameCount: " + impinfo[1]);// Debug
					if (impinfo[1] == 0)
					{
						Toast.makeText(HeadsetService.this, R.string.impfilefault, Toast.LENGTH_SHORT).show();
					}
					else
					{
						int arraySize2Send = 4096;
						impinfo[1] = impulseResponse.length / impinfo[0];
						int impulseCutted = impulseResponse.length;
						float[] sendArray = new float[arraySize2Send];
						int numTime2Send = (int)Math.ceil((double)impulseCutted / arraySize2Send); // Send number of times that have to send
						int[] sendBufInfo = new int[] {impulseCutted, impinfo[0], 0, numTime2Send};
						session.setParameterIntArray(session.JamesDSP, 9999, sendBufInfo); // Send buffer info for module to allocate memory
						float[] finalArray = new float[numTime2Send*arraySize2Send]; // Fill final array with zero padding
						System.arraycopy(impulseResponse, 0, finalArray, 0, impulseCutted);
						for (int i = 0; i < numTime2Send; i++)
						{
							System.arraycopy(finalArray, arraySize2Send * i, sendArray, 0, arraySize2Send);
							session.setParameterFloatArray(session.JamesDSP, 12000, sendArray); // Commit buffer
						}
						session.setParameterShort(session.JamesDSP, 10004, (short)1); // Notify send array completed and resize array in native side
						session.setParameterInt(session.JamesDSP, 25003, impinfo[2]); // Commit hashID to engine
						if (DSPManager.devMsgDisplay)
						{
							Toast.makeText(HeadsetService.this, getString(R.string.basicinfo, dspBufferLen, dspAllocatedBlockLen), Toast.LENGTH_SHORT).show();
							if (impinfo[0] == 1)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, getString(R.string.mono_conv), impinfo[1], (int)impulseCutted), Toast.LENGTH_SHORT).show();
							else if (impinfo[0] == 2)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, getString(R.string.stereo_conv), impinfo[1], (int)impulseCutted / 2), Toast.LENGTH_SHORT).show();
							else if (impinfo[0] == 4)
								Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName, getString(R.string.fullstereo_conv), impinfo[1], (int)impulseCutted / 4), Toast.LENGTH_SHORT).show();
						}
					}
		    	}
			}
			session.setParameterShort(session.JamesDSP, 1205, (short)convolverEnabled); // Convolver switch
		}
	}
}