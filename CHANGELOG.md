# Changelog

All notable changes to this package are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- `notifications_supported()`: whether the platform can show notifications, before asking.
  True on iOS, Android and macOS, in a browser with the Notification API (shown only while the
  page is open), on Linux when a session bus is there to reach a notification server, and on
  Windows.
- macOS: notifications through `UNUserNotificationCenter`, as on iOS.
- Linux: notifications on the desktop's notification server (`org.freedesktop.Notifications`)
  over D-Bus with GIO. A scheduled one shows only while the app runs.
- Windows: notifications as a balloon from the app's icon in the notification area. A
  scheduled one shows only while the app runs.
- CI builds the example on the native desktop hosts (`tools/ci_desktop.sh`).

### Fixed

- Native desktop builds of an app using the package link (before, the externs were missing on
  macOS, Linux and Windows).
