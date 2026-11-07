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

Android's alarms are not exact (exact ones need the exact alarm permission): the system may
batch one by a little, more when the device is idle.

## The example

`examples/reminders` has four reminders with switches. The first one arrives in ten seconds:

```bash
cd examples/reminders
ui run ios        # or android, web
```

## Tests

`eskiuc test` runs `tests/notifications.esk` on this computer, with stand-ins for the
platform's side: the permission as it is checked and asked, schedules and cancels, the
notification that opened the app, one opened while the app runs, and that one handled is not
handed out again.
