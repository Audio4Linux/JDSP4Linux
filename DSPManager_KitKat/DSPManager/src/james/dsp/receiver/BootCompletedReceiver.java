package james.dsp.receiver;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import james.dsp.service.HeadsetService;
public class BootCompletedReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        context.startService(new Intent(context, HeadsetService.class));
    }
}
