// The alarm of a scheduled notification (declared in the manifest through ui.toml): it posts
// the notification on the app's channel, with a tap that opens the app.
package org.eskiu.notifications;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

public final class NotificationAlarm extends BroadcastReceiver {
    private static final String CHANNEL = "notifications";

    @Override public void onReceive(Context context, Intent alarm) {
        String id = alarm.getStringExtra(EskiuNotifications.EXTRA);
        if (id == null) return;
        NotificationManager manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= 26 && manager.getNotificationChannel(CHANNEL) == null) {
            manager.createNotificationChannel(new NotificationChannel(CHANNEL, "Notifications", NotificationManager.IMPORTANCE_HIGH));
        }
        Intent open = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        open.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        open.putExtra(EskiuNotifications.EXTRA, id);
        PendingIntent tap = PendingIntent.getActivity(context, id.hashCode(), open,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        Notification.Builder builder = Build.VERSION.SDK_INT >= 26 ? new Notification.Builder(context, CHANNEL)
                                                                   : new Notification.Builder(context);
        builder.setSmallIcon(android.R.drawable.ic_dialog_info)
            .setContentTitle(alarm.getStringExtra("title"))
            .setContentText(alarm.getStringExtra("body"))
            .setContentIntent(tap)
            .setAutoCancel(true);
        manager.notify(id, 1, builder.build());
    }
}
