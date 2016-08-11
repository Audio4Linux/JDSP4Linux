package james.dsp.activity;

import android.app.Activity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.InputStream;
import java.io.IOException;
import java.io.SyncFailedException;
import java.util.Scanner;

public class Utils {
    protected static final String TAG = DSPManager.class.getSimpleName();
    private static final String TAG_READ = TAG + "_Read";
    private static final String TAG_WRITE = TAG + "_Write";

    /**
     * Write a string value to the specified file.
     *
     * @param filename The filename
     * @param value    The value
     */
    public static void writeValue(String filename, String value) {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(new File(filename), false);
            fos.write(value.getBytes());
            fos.flush();
        } catch (FileNotFoundException ex) {
        } catch (SyncFailedException ex) {
        } catch (IOException ex) {
        } catch (RuntimeException ex) {
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException ex) {
                } catch (RuntimeException ex) {
                }
            }
        }

    }

    /**
     * Write a string value to the specified file.
     *
     * @param filename The filename
     * @param value    The value
     */
    public static void writeValue(String filename, boolean value) {
        writeValue(filename, value ? "1" : "0");
    }

    /**
     * Write the "color value" to the specified file. The value is scaled from
     * an integer to an unsigned integer by multiplying by 2.
     *
     * @param filename The filename
     * @param value    The value of max value Integer.MAX
     */
    public static void writeColor(String filename, int value) {
        writeValue(filename, String.valueOf((long) value * 2));
    }

    /**
     * Check if the specified file exists.
     *
     * @param filename The filename
     * @return Whether the file exists or not
     */
    public static boolean fileExists(String filename) {
        return new File(filename).exists();
    }

    // Read value from sysfs interface
    public static String readOneLine(String sFile) {
        BufferedReader brBuffer;
        String sLine = null;

        try {
            brBuffer = new BufferedReader(new FileReader(sFile), 512);
            try {
                sLine = brBuffer.readLine();
            } finally {
                brBuffer.close();
            }
        } catch (Exception e) {
        }
        return sLine;
    }

    /**
     * Restart the activity smoothly
     *
     * @param activity
     */
    public static void restartActivity(final Activity activity) {
        if (activity == null)
            return;
        final int enter_anim = android.R.anim.fade_in;
        final int exit_anim = android.R.anim.fade_out;
        activity.overridePendingTransition(enter_anim, exit_anim);
        activity.finish();
        activity.overridePendingTransition(enter_anim, exit_anim);
        activity.startActivity(activity.getIntent());
    }

    /*
     * Loose catch block
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     */
    public static String getSystemFileString(String string) {
        try {
            Process process = new ProcessBuilder(new String[]{"cat", string}).start();
            String string2 = readAll(process.getInputStream());
            process.waitFor();
            process.destroy();
            return string2;
        } catch (IOException var3_3) {
            return "ERROR";
        } catch (InterruptedException var1_4) {
        }
        return "ERROR";
    }

    public static String readAll(InputStream inputStream) {
        StringBuilder stringBuilder = new StringBuilder();
        Scanner scanner = new Scanner(inputStream);
        while (scanner.hasNextLine()) {
            stringBuilder.append(scanner.nextLine());
        }
        return stringBuilder.toString();
    }
}
