package james.dsp.service;

import android.app.Service;
import android.bluetooth.BluetoothDevice;
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
import james.dsp.framework.BassBoost;
import james.dsp.framework.Equalizer;
import james.dsp.framework.StereoWide;
import james.dsp.framework.Virtualizer;
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
    protected static class EffectSet {
        private static final UUID EFFECT_TYPE_VOLUME = UUID
                .fromString("09e8ede0-ddde-11db-b4f6-0002a5d5c51b");
        private static final UUID EFFECT_TYPE_NULL = UUID
                .fromString("ec7178ec-e5e1-4432-a3f4-4657e6795210");

        /**
         * Session-specific dynamic range compressor
         */
        public final AudioEffect mCompression;
        /**
         * Session-specific equalizer
         */
        private final Equalizer mEqualizer;
        /**
         * Session-specific bassboost
         */
        private final BassBoost mBassBoost;
        /**
         * Session-specific virtualizer
         */
        private final Virtualizer mVirtualizer;
        /**
         * Session-specific stereo widener
         */
        private StereoWide mStereoWide;

        protected EffectSet(int sessionId) {
            try {
                /*
                 * AudioEffect constructor is not part of SDK. We use reflection
                 * to access it.
                 */
                mCompression = AudioEffect.class.getConstructor(UUID.class,
                        UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
                        EFFECT_TYPE_VOLUME, EFFECT_TYPE_NULL, 0, sessionId);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            mEqualizer = new Equalizer(0, sessionId);
            mBassBoost = new BassBoost(0, sessionId);
            mVirtualizer = new Virtualizer(0, sessionId);
            mStereoWide = new StereoWide(0, sessionId);
}

        protected void release() {
            mCompression.release();
            mEqualizer.release();
            mBassBoost.release();
            mVirtualizer.release();
            mStereoWide.release();
        }

        /**
         * Proxies call to AudioEffect.setParameter(byte[], byte[]) which is
         * available via reflection.
         *
         * @param audioEffect
         * @param parameter
         * @param value
         */
        private static void setParameter(AudioEffect audioEffect, int parameter, short value) {
            try {
                byte[] arguments = new byte[]{
                        (byte) (parameter), (byte) (parameter >> 8),
                        (byte) (parameter >> 16), (byte) (parameter >> 24)
                };
                byte[] result = new byte[]{
                        (byte) (value), (byte) (value >> 8)
                };

                Method setParameter = AudioEffect.class.getMethod(
                        "setParameter", byte[].class, byte[].class);
                int returnValue = (Integer) setParameter.invoke(audioEffect,
                        arguments, result);
            } catch (Exception e) {
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
    private final BroadcastReceiver mAudioSessionReceiver = new BroadcastReceiver() {
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
    private final BroadcastReceiver mPreferenceUpdateReceiver = new BroadcastReceiver() {
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
    private final BroadcastReceiver mRoutingReceiver = new BroadcastReceiver() {
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

    private final BroadcastReceiver mBtReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, final Intent intent) {
            final String action = intent.getAction();
            if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED)) {
                int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE,
                    BluetoothProfile.STATE_CONNECTED);

                if (state == BluetoothProfile.STATE_CONNECTED && !mUseBluetooth) {
                    mUseBluetooth = true;
                    updateDsp();
                } else if (mUseBluetooth) {
                    mUseBluetooth = false;
                    updateDsp();
                }
            } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
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
            } catch (Exception e) {
                mAudioSessions.remove(sessionId);
            }
        }
    }

    private void updateDsp(SharedPreferences preferences, EffectSet session) {
        session.mCompression.setEnabled(preferences.getBoolean("dsp.compression.enable", false));
        EffectSet.setParameter(session.mCompression, 0, Short.valueOf(preferences.getString("dsp.compression.mode", "0")));

            session.mBassBoost.setEnabled(preferences.getBoolean("dsp.bass.enable", false));
            session.mBassBoost.setStrength(Short.valueOf(preferences.getString("dsp.bass.mode", "0")));
	    session.mBassBoost.setFilterSlope(Short.valueOf(preferences.getString("dsp.bass.slope", "0")));
	    session.mBassBoost.setCenterFrequency(Short.valueOf(preferences.getString("dsp.bass.freq", "55")));
        /* Equalizer state is in a single string preference with all values separated by ; */
        session.mEqualizer.setEnabled(preferences.getBoolean("dsp.tone.enable", false));
        if (mOverriddenEqualizerLevels != null) {
            for (short i = 0; i < mOverriddenEqualizerLevels.length; i++) {
                session.mEqualizer.setBandLevel(i,
                        (short) Math.round(mOverriddenEqualizerLevels[i] * 100));
            }
        } else {
            String[] levels = preferences.getString("dsp.tone.eq.custom", "0;0;0;0;0;0;0;0;0;0;0;0").split(";");
            for (short i = 0; i < levels.length; i++) {
                session.mEqualizer.setBandLevel(i,
                        (short) Math.round(Float.valueOf(levels[i]) * 100));
            }
        }
        EffectSet.setParameter(session.mEqualizer, 1000, Short.valueOf(preferences.getString("dsp.tone.loudness", "10000")));
	    session.mEqualizer.setPreAmp(Short.valueOf(preferences.getString("dsp.tone.preamp", "1")));

        session.mVirtualizer.setEnabled(preferences.getBoolean("dsp.headphone.enable", false));
        session.mVirtualizer.setStrength(Short.valueOf(preferences.getString("dsp.headphone.mode", "0")));
	session.mVirtualizer.setEchoDecay(Short.valueOf(preferences.getString("dsp.headphone.echodecay", "1000")));
	session.mVirtualizer.setReverbMode(Short.valueOf(preferences.getString("dsp.headphone.modeverb", "1")));
	session.mVirtualizer.setRoomSize(Short.valueOf(preferences.getString("dsp.headphone.roomsize", "50")));
	session.mVirtualizer.setReverbTime(Short.valueOf(preferences.getString("dsp.headphone.reverbtime", "50")));
	session.mVirtualizer.setDamping(Short.valueOf(preferences.getString("dsp.headphone.damping", "50")));
	session.mVirtualizer.setSpread(Short.valueOf(preferences.getString("dsp.headphone.spread", "50")));
	session.mVirtualizer.setInBandwidth(Short.valueOf(preferences.getString("dsp.headphone.inbandwidth", "80")));
	session.mVirtualizer.setEarlyVerb(Short.valueOf(preferences.getString("dsp.headphone.earlyverb", "50")));
	session.mVirtualizer.setTailVerb(Short.valueOf(preferences.getString("dsp.headphone.tailverb", "50")));
	session.mVirtualizer.setWetMix(Short.valueOf(preferences.getString("dsp.headphone.wetmix", "80")));

            session.mStereoWide.setEnabled(preferences.getBoolean("dsp.stereowide.enable", false));
            session.mStereoWide.setStrength(Short.valueOf(preferences.getString("dsp.stereowide.mode", "0")));
    }
}
