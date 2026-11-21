# Changelog

All notable changes to this package are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- `notifications_supported()`: whether the platform can show notifications, before asking.
  True on iOS and Android and in a browser with the Notification API (shown only while the
  page is open); false on the desktop.
