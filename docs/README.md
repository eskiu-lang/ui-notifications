# ui-notifications documentation

Guides and reference for ui-notifications. The [README](../README.md) has the overview.

<!-- pages -->
- [API reference](api.md): every public function and type
<!-- /pages -->

## Platforms

| Platform | Scheduling | Opening |
|---|---|---|
| iOS | `UNTimeIntervalNotificationTrigger`; a notification that arrives while the app is in front shows as a banner too | The delegate is set before the app finishes launching, so one that opens a closed app reaches it |
| Android | `AlarmManager.setAndAllowWhileIdle` wakes `NotificationAlarm`, which posts on the app's channel (high importance, so it pops up) | The tap opens the app with the id in the intent, also from a closed app |
| Web | A timer in the page: it shows only while the page is open | A click focuses the page |
| macOS | `UNUserNotificationCenter`, as on iOS; macOS shows it at its time also when the app has quit | The delegate is set before the app finishes launching, as on iOS |
| Linux | A timer in the app, then `Notify` on the desktop's notification server (`org.freedesktop.Notifications`, over D-Bus with GIO): it shows only while the app runs | A click (the default action) brings the window up |
| Windows | A timer in the app, then a balloon from the app's icon in the notification area (`Shell_NotifyIcon`), which Windows 10 and 11 show as a notification: it shows only while the app runs | A click on the balloon brings the window up |

The permission on the desktops: macOS asks the user as iOS does. Linux has no permission to
ask for: the app is allowed when a notification server answers on the session bus, and denied
when none does. Windows has none either: the app is denied when the user turned notifications
off in the system's settings, and allowed otherwise.

Windows shows a balloon, not a toast: a toast needs the WinRT API and a Start menu shortcut
carrying the app's id, which an app built with MinGW-w64 does not have. A balloon looks like a
notification, but only the newest one shows (a new balloon replaces the one before), it lands
in the notification center under the app's icon, and the icon stays in the notification area
while a notification waits or shows.

The desktop's `--webview` builds use the web's Notification API, through the webview.

Android's alarms are not exact (exact ones need the exact alarm permission): the system may
batch one by a little, more when the device is idle.

## The example

`examples/reminders` has four reminders with switches. The first one arrives in ten seconds:

```bash
cd examples/reminders
ui run ios        # or android, web, macos, linux, windows
```

## Tests

`eskiuc test` runs `tests/notifications.esk` on this computer, with stand-ins for the
platform's side: the permission as it is checked and asked, schedules and cancels, the
notification that opened the app, one opened while the app runs, and that one handled is not
handed out again.
