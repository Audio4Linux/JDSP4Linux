package james.dsp.activity;

import android.util.Log;

public class JdspImpResToolbox
{
    static
    {
    	try {
    		System.loadLibrary("jamesDSPImpulseToolbox");
    		Log.i(DSPManager.TAG, "JNI loaded via normal installation");
    	} catch(UnsatisfiedLinkError e) {
    			Log.e(DSPManager.TAG, "JNI LOAD FAILED!");
    	}
    }
    /* Impulse Response Utils */
    public static native int[] GetLoadImpulseResponseInfo(String path);
    public static native float[] ReadImpulseResponseToFloat(int targetSampleRate);
    public static native String OfflineAudioResample(String path, String filename, int targetSampleRate);
}
