// <notifications> on Android: AlarmManager wakes NotificationAlarm at the time, which posts the
// notification; tapping it opens the app with the notification's id in the intent.
// notifications.c calls these from Eskiu; answers go to the "notifications" channel.
package org.eskiu.notifications;

import android.Manifest;
import android.app.Activity;
import android.app.AlarmManager;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import org.eskiu.ui.UiActivity;

public final class EskiuNotifications {
    static final String EXTRA = "org.eskiu.notifications.id";
    // The id of a notification that opened the app, until the app takes it.
    private static String opened = "";

    public static void setup() {
        UiActivity.onIntent(intent -> {
            String id = intent == null ? null : intent.getStringExtra(EXTRA);
            if (id == null) return;
            intent.removeExtra(EXTRA);
            opened = id;
            UiActivity.emit("notifications", "opened " + id);
        });
    }

    private static boolean allowed(Context context) {
        NotificationManager manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        return manager.areNotificationsEnabled() && (Build.VERSION.SDK_INT < 33 ||
            context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == android.content.pm.PackageManager.PERMISSION_GRANTED);
    }

    public static void check() {
        Activity activity = UiActivity.current();
        if (activity == null) return;
        String state = allowed(activity) ? "allowed" : Build.VERSION.SDK_INT >= 33 ? "notasked" : "denied";
        UiActivity.emit("notifications", "permission " + state);
    }

    public static void request() {
        Activity activity = UiActivity.current();
        if (activity == null) return;
        if (Build.VERSION.SDK_INT < 33) { check(); return; }
        UiActivity.requestPermissions(new String[] {Manifest.permission.POST_NOTIFICATIONS},
            granted -> UiActivity.emit("notifications", "permission " + (granted ? "allowed" : "denied")));
    }

    private static PendingIntent alarm(Context context, String id, String title, String body) {
        Intent intent = new Intent(context, NotificationAlarm.class);
        intent.setAction("org.eskiu.notifications.ALARM:" + id);
        intent.putExtra(EXTRA, id);
        intent.putExtra("title", title);
        intent.putExtra("body", body);
        return PendingIntent.getBroadcast(context, id.hashCode(), intent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
    }

    public static void schedule(String id, String title, String body, int seconds) {
        Activity activity = UiActivity.current();
        if (activity == null) return;
        AlarmManager alarms = (AlarmManager)activity.getSystemService(Context.ALARM_SERVICE);
        // Not exact (that needs the exact alarm permission): the system may batch it a little.
        alarms.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + seconds * 1000L,
            alarm(activity, id, title, body));
    }

    public static void cancel(String id) {
        Activity activity = UiActivity.current();
        if (activity == null) return;
        ((AlarmManager)activity.getSystemService(Context.ALARM_SERVICE)).cancel(alarm(activity, id, "", ""));
        ((NotificationManager)activity.getSystemService(Context.NOTIFICATION_SERVICE)).cancel(id, 1);
    }

    public static String takeOpened() {
        String id = opened;
        opened = "";
        return id;
    }
}
