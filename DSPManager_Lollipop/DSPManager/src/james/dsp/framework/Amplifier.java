package james.dsp.framework;

import android.media.audiofx.AudioEffect;
import java.util.StringTokenizer;
import java.util.UUID;
/**
 * Amplifier is an audio effect to boost or amplify low frequencies of the sound. It is comparable
 * to a simple equalizer but limited to one band amplification in the low frequency range.
 * <p>An application creates a Amplifier object to instantiate and control a Amplifier engine in
 * the audio framework.
 * <p>The methods, parameter types and units exposed by the Amplifier implementation are directly
 * mapping those defined by the OpenSL ES 1.0.1 Specification (http://www.khronos.org/opensles/)
 * for the SLAmplifierItf interface. Please refer to this specification for more details.
 * <p>To attach the Amplifier to a particular AudioTrack or MediaPlayer, specify the audio session
 * ID of this AudioTrack or MediaPlayer when constructing the Amplifier.
 * <p>NOTE: attaching a Amplifier to the global audio output mix by use of session 0 is deprecated.
 * <p>See {@link android.media.MediaPlayer#getAudioSessionId()} for details on audio sessions.
 * <p>See {@link android.media.audiofx.AudioEffect} class for more details on
 * controlling audio effects.
 */

public class Amplifier extends AudioEffect {

    private final static String TAG = "Amplifier";

    // These constants must be synchronized with those in
    // frameworks/base/include/media/EffectAmplifierApi.h
    /**
     * Is strength parameter supported by Amplifier engine. Parameter ID for getParameter().
     */
    public static final int PARAM_STRENGTH_SUPPORTED = 0;
    /**
     * Amplifier effect strength. Parameter ID for
     * {@link android.media.audiofx.Amplifier.OnParameterChangeListener}
     */
    public static final int PARAM_STRENGTH = 1;
    public static final UUID EFFECT_TYPE_AMPLIFIER = UUID.fromString("98c8baf0-23a4-4ce8-8699-6b1853969e9f");
    /**
     * Amplifier center frequency. Paremeter ID for
     * {@link android.media.audiofx.Amplifier.OnParameterChangeListener}
     * @hide
     */
    private boolean mStrengthSupported = false;

    /**
     * Registered listener for parameter changes.
     */
    private OnParameterChangeListener mParamListener = null;

    /**
     * Listener used internally to to receive raw parameter change event from AudioEffect super class
     */
    private BaseParameterListener mBaseParamListener = null;

    /**
     * Lock for access to mParamListener
     */
    private final Object mParamListenerLock = new Object();

    /**
     * Class constructor.
     * @param priority the priority level requested by the application for controlling the Amplifier
     * engine. As the same engine can be shared by several applications, this parameter indicates
     * how much the requesting application needs control of effect parameters. The normal priority
     * is 0, above normal is a positive number, below normal a negative number.
     * @param audioSession system wide unique audio session identifier. The Amplifier will be
     * attached to the MediaPlayer or AudioTrack in the same audio session.
     *
     * @throws java.lang.IllegalStateException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.UnsupportedOperationException
     * @throws java.lang.RuntimeException
     */
    public Amplifier(int priority, int audioSession)
    throws IllegalStateException, IllegalArgumentException,
           UnsupportedOperationException, RuntimeException {
        super(EFFECT_TYPE_AMPLIFIER, EFFECT_TYPE_NULL, priority, audioSession);

        if (audioSession == 0) {
        }

        int[] value = new int[1];
        checkStatus(getParameter(PARAM_STRENGTH_SUPPORTED, value));
        mStrengthSupported = (value[0] != 0);
    }

    /**
     * Indicates whether setting strength is supported. If this method returns false, only one
     * strength is supported and the setStrength() method always rounds to that value.
     * @return true is strength parameter is supported, false otherwise
     */
    public boolean getStrengthSupported() {
       return mStrengthSupported;
    }

    /**
     * Sets the strength of the Amplifier effect. If the implementation does not support per mille
     * accuracy for setting the strength, it is allowed to round the given strength to the nearest
     * supported value. You can use the {@link #getRoundedStrength()} method to query the
     * (possibly rounded) value that was actually set.
     * @param strength strength of the effect. The valid range for strength strength is [0, 1000],
     * where 0 per mille designates the mildest effect and 1000 per mille designates the strongest.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    public void setStrength(short strength)
    throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkStatus(setParameter(PARAM_STRENGTH, strength));
    }

    /**
     * Gets the current strength of the effect.
     * @return the strength of the effect. The valid range for strength is [0, 1000], where 0 per
     * mille designates the mildest effect and 1000 per mille the strongest
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    public short getRoundedStrength()
    throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        short[] value = new short[1];
        checkStatus(getParameter(PARAM_STRENGTH, value));
        return value[0];
    }

    /**
     * The OnParameterChangeListener interface defines a method called by the Amplifier when a
     * parameter value has changed.
     */
    public interface OnParameterChangeListener  {
        /**
         * Method called when a parameter value has changed. The method is called only if the
         * parameter was changed by another application having the control of the same
         * Amplifier engine.
         * @param effect the Amplifier on which the interface is registered.
         * @param status status of the set parameter operation.
         * @param param ID of the modified parameter. See {@link #PARAM_STRENGTH} ...
         * @param value the new parameter value.
         */
        void onParameterChange(Amplifier effect, int status, int param, short value);
    }

    /**
     * Listener used internally to receive unformatted parameter change events from AudioEffect
     * super class.
     */
    private class BaseParameterListener implements AudioEffect.OnParameterChangeListener {
        private BaseParameterListener() {

        }
        public void onParameterChange(AudioEffect effect, int status, byte[] param, byte[] value) {
            OnParameterChangeListener l = null;

            synchronized (mParamListenerLock) {
                if (mParamListener != null) {
                    l = mParamListener;
                }
            }
            if (l != null) {
                int p = -1;
                short v = -1;

                if (param.length == 4) {
                    p = byteArrayToInt(param, 0);
                }
                if (value.length == 2) {
                    v = byteArrayToShort(value, 0);
                }
                if (p != -1 && v != -1) {
                    l.onParameterChange(Amplifier.this, status, p, v);
                }
            }
        }
    }

    /**
     * Registers an OnParameterChangeListener interface.
     * @param listener OnParameterChangeListener interface registered
     */
    public void setParameterListener(OnParameterChangeListener listener) {
        synchronized (mParamListenerLock) {
            if (mParamListener == null) {
                mParamListener = listener;
                mBaseParamListener = new BaseParameterListener();
                super.setParameterListener(mBaseParamListener);
            }
        }
    }

    /**
     * The Settings class regroups all Amplifier parameters. It is used in
     * conjuntion with getProperties() and setProperties() methods to backup and restore
     * all parameters in a single call.
     */
    public static class Settings {
        public short strength;

        public Settings() {
        }

        /**
         * Settings class constructor from a key=value; pairs formatted string. The string is
         * typically returned by Settings.toString() method.
         * @throws IllegalArgumentException if the string is not correctly formatted.
         */
        public Settings(String settings) {
            StringTokenizer st = new StringTokenizer(settings, "=;");
            int tokens = st.countTokens();
            if (st.countTokens() != 3) {
                throw new IllegalArgumentException("settings: " + settings);
            }
            String key = st.nextToken();
            if (!key.equals("Amplifier")) {
                throw new IllegalArgumentException(
                        "invalid settings for Amplifier: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("strength")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                strength = Short.parseShort(st.nextToken());
             } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            String str = new String (
                    "Amplifier"+
                    ";strength="+Short.toString(strength)
                    );
            return str;
        }
    };


    /**
     * Gets the Amplifier properties. This method is useful when a snapshot of current
     * Amplifier settings must be saved by the application.
     * @return a Amplifier.Settings object containing all current parameters values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    public Amplifier.Settings getProperties()
    throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        Settings settings = new Settings();
        short[] value = new short[1];
        checkStatus(getParameter(PARAM_STRENGTH, value));
        settings.strength = value[0];
        return settings;
    }

    /**
     * Sets the Amplifier properties. This method is useful when Amplifier settings have to
     * be applied from a previous backup.
     * @param settings a Amplifier.Settings object containing the properties to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    public void setProperties(Amplifier.Settings settings)
    throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkStatus(setParameter(PARAM_STRENGTH, settings.strength));
    }
}

