package james.dsp.activity;

import android.util.Log;

public class JdspImpResToolbox
{
    static
    {
    	try {
    		System.loadLibrary("jamesDSPImpulseToolbox");
    	} catch(UnsatisfiedLinkError e) {
    		try {
    			System.load("/system/lib/libjamesDSPImpulseToolbox.so");
    		} catch(UnsatisfiedLinkError error) {
    			Log.e(DSPManager.TAG, "JNI LOAD FAILED!");
    		}
    	}
    }
    /* Impulse Response Utils */
    public static native int[] GetLoadImpulseResponseInfo(String path);
    public static native int[] ReadImpulseResponseToInt(int targetSampleRate);
    public static native String OfflineAudioResample(String path, String filename, int targetSampleRate);
    public static native int FFTConvolutionBenchmark(int entriesGen, int fs, double c0[], double c1[]);
    public static native String FFTConvolutionBenchmarkToString(int entriesGen, int fs);
}
