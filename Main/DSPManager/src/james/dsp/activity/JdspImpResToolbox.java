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
    public static native float[] ReadImpulseResponseToFloat(String path, int targetSampleRate, int audioInfo[], int convMode, int advParam[]);
    public static native String OfflineAudioResample(String path, String filename, int targetSampleRate);
    public static native int ComputeEqResponse(int n, double freq[], double gain[], int interpolationMode, int queryPts, double dispFreq[], float response[]);
    public static native int ComputeCompResponse(int n, double freq[], double gain[], int queryPts, double dispFreq[], float response[]);
}
