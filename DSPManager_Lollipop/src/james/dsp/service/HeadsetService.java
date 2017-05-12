package james.dsp.service;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.media.audiofx.AudioEffect;
import android.os.Binder;
import android.os.IBinder;
import james.dsp.activity.DSPManager;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
* <p>This calls listen to events that affect DSP function and responds to them.</p>
* <ol>
* <li>new audio session declarations</li>
* <li>headset plug / unplug events</li>
* <li>preference update events.</li>
* </ol>
*
* @author alankila
*/
public class HeadsetService extends Service {
	/**
	* Helper class representing the full complement of effects attached to one
	* audio session.
	*
	* @author alankila
	*/
	//    private static short impData[];
	protected static class EffectSet {
		private static final UUID EFFECT_TYPE_VOLUME = UUID.fromString("09e8ede0-ddde-11db-b4f6-0002a5d5c51b");
		private static final UUID EFFECT_TYPE_NULL = UUID.fromString("ec7178ec-e5e1-4432-a3f4-4657e6795210");
		public AudioEffect JamesDSP;

		protected EffectSet(int sessionId) {
			try {
				/*
				* AudioEffect constructor is not part of SDK. We use reflection
				* to access it.
				*/
				JamesDSP = AudioEffect.class.getConstructor(UUID.class,
					UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
						EFFECT_TYPE_VOLUME, EFFECT_TYPE_NULL, 0, sessionId);
			}
			catch (Exception e) {
				throw new RuntimeException(e);
			}
			/*            impData = new short[8192];
			for(int i = 0; i < 8192; i++)
			impData[i] = (short) (i * 3);*/
		}

		protected void release() {
			JamesDSP.release();
		}
		private static byte[] ShortToByte(short[] input)
		{
			int short_index, byte_index;
			int iterations = input.length;

			byte[] buffer = new byte[input.length * 2];

			short_index = byte_index = 0;

			for (; short_index != iterations;)
			{
				buffer[byte_index] = (byte)(input[short_index] & 0x00FF);
				buffer[byte_index + 1] = (byte)((input[short_index] & 0xFF00) >> 8);

				++short_index; byte_index += 2;
			}

			return buffer;
		}
		/**
		* Proxies call to AudioEffect.setParameter(byte[], byte[]) which is
		* available via reflection.
		*
		* @param audioEffect
		* @param parameter
		* @param value
		*/
		private static void setParameterArray(AudioEffect audioEffect, int parameter, short value[]) {
			try {
				byte[] arguments = new byte[]{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = ShortToByte(value);

				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e) {
				throw new RuntimeException(e);
			}
		}
		private static void setParameter(AudioEffect audioEffect, int parameter, short value) {
			try {
				byte[] arguments = new byte[]{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[]{
					(byte)(value), (byte)(value >> 8)
				};

				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e) {
				throw new RuntimeException(e);
			}
		}
	}

	private static final String TAG = "DSPManager";

	public class LocalBinder extends Binder {
		public HeadsetService getService() {
			return HeadsetService.this;
		}
	}

	private final LocalBinder mBinder = new LocalBinder();

	/**
	* Known audio sessions and their associated audioeffect suites.
	*/
	protected final Map<Integer, EffectSet> mAudioSessions = new HashMap<Integer, EffectSet>();

	/**
	* Is a wired headset plugged in?
	*/
	protected boolean mUseHeadset;

	/**
	* Is bluetooth headset plugged in?
	*/
	protected boolean mUseBluetooth;

	/**
	* Has DSPManager assumed control of equalizer levels?
	*/
	private float[] mOverriddenEqualizerLevels;

	/**
	* Receive new broadcast intents for adding DSP to session
	*/
	private final BroadcastReceiver mAudioSessionReceiver = new BroadcastReceiver(){
		@Override
		public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		int sessionId = intent.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION, 0);
		if (action.equals(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION)) {
			if (!mAudioSessions.containsKey(sessionId)) {
				mAudioSessions.put(sessionId, new EffectSet(sessionId));
			}
		}
		if (action.equals(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION)) {
			EffectSet gone = mAudioSessions.remove(sessionId);
			if (gone != null) {
				gone.release();
			}
		}
		updateDsp();
	}
	};

	/**
	* Update audio parameters when preferences have been updated.
	*/
	private final BroadcastReceiver mPreferenceUpdateReceiver = new BroadcastReceiver(){
		@Override
		public void onReceive(Context context, Intent intent) {
		updateDsp();
	}
	};

	/**
	* This code listens for changes in bluetooth and headset events. It is
	* adapted from google's own MusicFX application, so it's presumably the
	* most correct design there is for this problem.
	*/
	private final BroadcastReceiver mRoutingReceiver = new BroadcastReceiver(){
		@Override
		public void onReceive(final Context context, final Intent intent) {
		final String action = intent.getAction();
		final boolean prevUseHeadset = mUseHeadset;
		if (action.equals(AudioManager.ACTION_HEADSET_PLUG)) {
			mUseHeadset = intent.getIntExtra("state", 0) == 1;
		}
		if (prevUseHeadset != mUseHeadset) {
			updateDsp();
		}
	}
	};

	private final BroadcastReceiver mBtReceiver = new BroadcastReceiver(){
		@Override
		public void onReceive(final Context context, final Intent intent) {
		final String action = intent.getAction();
		if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED)) {
			int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE,
				BluetoothProfile.STATE_CONNECTED);

			if (state == BluetoothProfile.STATE_CONNECTED && !mUseBluetooth) {
				mUseBluetooth = true;
				updateDsp();
			}
			else if (mUseBluetooth) {
				mUseBluetooth = false;
				updateDsp();
			}
		}
		else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
			String stateExtra = BluetoothAdapter.EXTRA_STATE;
			int state = intent.getIntExtra(stateExtra, -1);

			if (state == BluetoothAdapter.STATE_OFF && mUseBluetooth) {
				mUseBluetooth = false;
				updateDsp();
			}
		}
	}
	};

	@Override
		public void onCreate() {
		super.onCreate();

		IntentFilter audioFilter = new IntentFilter();
		audioFilter.addAction(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION);
		audioFilter.addAction(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION);
		registerReceiver(mAudioSessionReceiver, audioFilter);

		final IntentFilter intentFilter = new IntentFilter(AudioManager.ACTION_HEADSET_PLUG);
		registerReceiver(mRoutingReceiver, intentFilter);

		registerReceiver(mPreferenceUpdateReceiver,
			new IntentFilter(DSPManager.ACTION_UPDATE_PREFERENCES));

		final IntentFilter btFilter = new IntentFilter();
		btFilter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
		btFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
		registerReceiver(mBtReceiver, btFilter);
	}

	@Override
		public void onDestroy() {
		super.onDestroy();

		unregisterReceiver(mAudioSessionReceiver);
		unregisterReceiver(mRoutingReceiver);
		unregisterReceiver(mPreferenceUpdateReceiver);
	}

	@Override
		public IBinder onBind(Intent intent) {
		return mBinder;
	}

	/**
	* Gain temporary control over the global equalizer.
	* Used by DSPManager when testing a new equalizer setting.
	*
	* @param levels
	*/
	public void setEqualizerLevels(float[] levels) {
		mOverriddenEqualizerLevels = levels;
		updateDsp();
	}

	/**
	* There appears to be no way to find out what the current actual audio routing is.
	* For instance, if a wired headset is plugged in, the following objects/classes are involved:</p>
	* <ol>
	* <li>wiredaccessoryobserver</li>
	* <li>audioservice</li>
	* <li>audiosystem</li>
	* <li>audiopolicyservice</li>
	* <li>audiopolicymanager</li>
	* </ol>
	* <p>Once the decision of new routing has been made by the policy manager, it is relayed to
	* audiopolicyservice, which waits for some time to let application buffers drain, and then
	* informs it to hardware. The full chain is:</p>
	* <ol>
	* <li>audiopolicymanager</li>
	* <li>audiopolicyservice</li>
	* <li>audiosystem</li>
	* <li>audioflinger</li>
	* <li>audioeffect (if any)</li>
	* </ol>
	* <p>However, the decision does not appear to be relayed to java layer, so we must
	* make a guess about what the audio output routing is.</p>
	*
	* @return string token that identifies configuration to use
	*/
	public String getAudioOutputRouting() {
		if (mUseBluetooth) {
			return "bluetooth";
		}
		if (mUseHeadset) {
			return "headset";
		}
		return "speaker";
	}

	/**
	* Push new configuration to audio stack.
	*/
	protected void updateDsp() {
		final String mode = getAudioOutputRouting();
		SharedPreferences preferences =
			getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + mode, 0);

		for (Integer sessionId : new ArrayList<Integer>(mAudioSessions.keySet())) {
			try {
				updateDsp(preferences, mAudioSessions.get(sessionId));
			}
			catch (Exception e) {
				mAudioSessions.remove(sessionId);
			}
		}
	}

	private void updateDsp(SharedPreferences preferences, EffectSet session) {
		session.JamesDSP.setEnabled(preferences.getBoolean("dsp.masterswitch.enable", false)); // Master switch
		int compressorEnabled = (preferences.getBoolean("dsp.compression.enable", false) ? 1 : 0);
		int bassBoostEnabled = (preferences.getBoolean("dsp.bass.enable", false) ? 1 : 0);
		int equalizerEnabled = (preferences.getBoolean("dsp.tone.enable", false) ? 1 : 0);
		int reverbEnabled = (preferences.getBoolean("dsp.headphone.enable", false) ? 1 : 0);
		int stereoEnabled = (preferences.getBoolean("dsp.stereowide.enable", false) ? 1 : 0);
		EffectSet.setParameter(session.JamesDSP, 1500, Short.valueOf(preferences.getString("dsp.masterswitch.clipmode", "1")));
		EffectSet.setParameter(session.JamesDSP, 1501, (short)(preferences.getBoolean("dsp.masterswitch.normalise", false) ? 1 : 0));
		EffectSet.setParameter(session.JamesDSP, 1502, Short.valueOf(preferences.getString("dsp.masterswitch.finalgain", "100")));
		if (compressorEnabled == 1)
		{
			EffectSet.setParameter(session.JamesDSP, 100, Short.valueOf(preferences.getString("dsp.compression.pregain", "0")));
			EffectSet.setParameter(session.JamesDSP, 101, Short.valueOf(preferences.getString("dsp.compression.threshold", "20")));
			EffectSet.setParameter(session.JamesDSP, 102, Short.valueOf(preferences.getString("dsp.compression.knee", "30")));
			EffectSet.setParameter(session.JamesDSP, 103, Short.valueOf(preferences.getString("dsp.compression.ratio", "12")));
			EffectSet.setParameter(session.JamesDSP, 104, Short.valueOf(preferences.getString("dsp.compression.attack", "30")));
			EffectSet.setParameter(session.JamesDSP, 105, Short.valueOf(preferences.getString("dsp.compression.release", "250")));
			EffectSet.setParameter(session.JamesDSP, 106, Short.valueOf(preferences.getString("dsp.compression.predelay", "60")));
			EffectSet.setParameter(session.JamesDSP, 107, Short.valueOf(preferences.getString("dsp.compression.releasezone1", "100")));
			EffectSet.setParameter(session.JamesDSP, 108, Short.valueOf(preferences.getString("dsp.compression.releasezone2", "150")));
			EffectSet.setParameter(session.JamesDSP, 109, Short.valueOf(preferences.getString("dsp.compression.releasezone3", "400")));
			EffectSet.setParameter(session.JamesDSP, 110, Short.valueOf(preferences.getString("dsp.compression.releasezone4", "950")));
			EffectSet.setParameter(session.JamesDSP, 111, Short.valueOf(preferences.getString("dsp.compression.postgain", "0")));
		}
		EffectSet.setParameter(session.JamesDSP, 1200, (short)compressorEnabled); // Compressor switch
		if (bassBoostEnabled == 1)
		{
			EffectSet.setParameter(session.JamesDSP, 112, Short.valueOf(preferences.getString("dsp.bass.mode", "80")));
			EffectSet.setParameter(session.JamesDSP, 113, Short.valueOf(preferences.getString("dsp.bass.slope", "0")));
			EffectSet.setParameter(session.JamesDSP, 114, Short.valueOf(preferences.getString("dsp.bass.freq", "55")));
		}
		EffectSet.setParameter(session.JamesDSP, 1201, (short)bassBoostEnabled); // Bass boost switch
		if (equalizerEnabled == 1)
		{
			/* Equalizer state is in a single string preference with all values separated by ; */
			if (mOverriddenEqualizerLevels != null) {
				for (short i = 0; i < mOverriddenEqualizerLevels.length; i++) {
					EffectSet.setParameter(session.JamesDSP, 115 + i, (short)Math.round(mOverriddenEqualizerLevels[i] * 100));
				}
			}
			else {
				String[] levels = preferences.getString("dsp.tone.eq.custom", "0;0;0;0;0;0;0;0;0;0").split(";");
				for (short i = 0; i < levels.length; i++) {
					EffectSet.setParameter(session.JamesDSP, 115 + i, (short)Math.round(Float.valueOf(levels[i]) * 100));
				}
			}
		}
		EffectSet.setParameter(session.JamesDSP, 1202, (short)equalizerEnabled); // Equalizer switch
		if (reverbEnabled == 1)
		{
			EffectSet.setParameter(session.JamesDSP, 127, Short.valueOf(preferences.getString("dsp.headphone.modeverb", "1")));
			EffectSet.setParameter(session.JamesDSP, 128, Short.valueOf(preferences.getString("dsp.headphone.preset", "0")));
			EffectSet.setParameter(session.JamesDSP, 129, Short.valueOf(preferences.getString("dsp.headphone.roomsize", "50")));
			EffectSet.setParameter(session.JamesDSP, 130, Short.valueOf(preferences.getString("dsp.headphone.reverbtime", "50")));
			EffectSet.setParameter(session.JamesDSP, 131, Short.valueOf(preferences.getString("dsp.headphone.damping", "50")));
			EffectSet.setParameter(session.JamesDSP, 132, Short.valueOf(preferences.getString("dsp.headphone.spread", "50")));
			EffectSet.setParameter(session.JamesDSP, 133, Short.valueOf(preferences.getString("dsp.headphone.inbandwidth", "80")));
			EffectSet.setParameter(session.JamesDSP, 134, Short.valueOf(preferences.getString("dsp.headphone.earlyverb", "50")));
			EffectSet.setParameter(session.JamesDSP, 135, Short.valueOf(preferences.getString("dsp.headphone.tailverb", "50")));
		}
		EffectSet.setParameter(session.JamesDSP, 1203, (short)reverbEnabled); // Reverb switch
		if (stereoEnabled == 1)
			EffectSet.setParameter(session.JamesDSP, 137, Short.valueOf(preferences.getString("dsp.stereowide.mode", "0")));
		EffectSet.setParameter(session.JamesDSP, 1204, (short)stereoEnabled); // Stereo widener switch
		/*   if((preferences.getBoolean("dsp.convolver.enable", false) ? 1 : 0) == 1) {
		EffectSet.setParameterArray(session.JamesDSP, 13000, impData); // Send buffer works!
		}*/
//		EffectSet.setParameter(session.JamesDSP, 1205, (short)(preferences.getBoolean("dsp.convolver.enable", false) ? 1 : 0)); // Convolver switch
	}
}
