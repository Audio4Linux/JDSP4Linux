package james.dsp.activity;

public class JdspImpResToolbox
{

    static
    {
        System.loadLibrary("jamesDSPImpulseToolbox");
    }

    /* Impulse Response Utils */
    public static native int[] GetLoadImpulseResponseInfo(String path);
    public static native int[] ReadImpulseResponseToInt();
}
