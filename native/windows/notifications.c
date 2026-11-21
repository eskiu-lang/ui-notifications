// <notifications> on Windows: a balloon from the app's icon in the notification area
// (Shell_NotifyIcon), which Windows 10 and 11 show as a notification. Not a toast: those need
// the WinRT API and a Start menu shortcut with the app's id, which a MinGW-built app does not
// have. A scheduled notification is a timer in the app, so it shows only while the app runs,
// as on the web, and the icon stays in the notification area while one waits or shows. A click
// on the balloon brings the window up and tells the app. Only the newest one shows: a balloon
// replaces the one before. There is no permission to ask for; the app is "denied" when the user
// turned notifications off in the system's settings.
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include "ui_native.h"

#define WM_TRAY (WM_APP + 0x4e5)
#define kIconId 1
#define kMaxScheduled 64

typedef struct {
    char id[128];
    wchar_t title[64];
    wchar_t body[256];
    UINT_PTR timer;   // 0 when the slot is free
} Scheduled;

static HWND owner;
static Scheduled waiting[kMaxScheduled];
static UINT_PTR nextTimer = 1;
static int iconShown;
static char showing[128];   // the id of the balloon that shows, "" for none
static char opened[128];    // the id of a notification the user opened, until taken

static void widen(const char *text, wchar_t *out, int size) {
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, out, size) == 0) {
        out[0] = 0;
        // Too long: cut it, keeping the end of the buffer for the terminator.
        MultiByteToWideChar(CP_UTF8, 0, text, size - 1, out, size - 1);
        out[size - 1] = 0;
    }
}

static void iconData(NOTIFYICONDATAW *data) {
    memset(data, 0, sizeof *data);
    data->cbSize = sizeof *data;
    data->hWnd = owner;
    data->uID = kIconId;
}

static int anyWaiting(void) {
    for (int i = 0; i < kMaxScheduled; i++) {
        if (waiting[i].timer) return 1;
    }
    return 0;
}

static void addIcon(void) {
    if (iconShown) return;
    NOTIFYICONDATAW data;
    iconData(&data);
    data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    data.uCallbackMessage = WM_TRAY;
    HWND window = ui_native_window();
    HICON icon = window ? (HICON)SendMessageW(window, WM_GETICON, ICON_SMALL, 0) : NULL;
    if (!icon && window) icon = (HICON)GetClassLongPtrW(window, GCLP_HICONSM);
    data.hIcon = icon ? icon : LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    if (window) GetWindowTextW(window, data.szTip, sizeof data.szTip / sizeof data.szTip[0]);
    iconShown = Shell_NotifyIconW(NIM_ADD, &data);
}

static void removeIcon(void) {
    if (!iconShown) return;
    NOTIFYICONDATAW data;
    iconData(&data);
    Shell_NotifyIconW(NIM_DELETE, &data);
    iconShown = 0;
    showing[0] = 0;
}

static void show(Scheduled *s) {
    addIcon();
    NOTIFYICONDATAW data;
    iconData(&data);
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
    wcsncpy(data.szInfoTitle, s->title, sizeof data.szInfoTitle / sizeof data.szInfoTitle[0] - 1);
    wcsncpy(data.szInfo, s->body[0] ? s->body : L" ", sizeof data.szInfo / sizeof data.szInfo[0] - 1);
    if (Shell_NotifyIconW(NIM_MODIFY, &data)) snprintf(showing, sizeof showing, "%s", s->id);
}

static LRESULT CALLBACK ownerProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_TIMER) {
        KillTimer(window, wparam);
        for (int i = 0; i < kMaxScheduled; i++) {
            if (waiting[i].timer == wparam) {
                waiting[i].timer = 0;
                show(&waiting[i]);
                break;
            }
        }
        return 0;
    }
    if (message == WM_TRAY) {
        UINT event = LOWORD(lparam);
        if (event == NIN_BALLOONUSERCLICK && showing[0]) {
            snprintf(opened, sizeof opened, "%s", showing);
            HWND main = ui_native_window();
            if (main) {
                if (IsIconic(main)) ShowWindow(main, SW_RESTORE);
                SetForegroundWindow(main);
            }
            char payload[160];
            snprintf(payload, sizeof payload, "opened %s", showing);
            ui_native_emit("notifications", payload);
        }
        if (event == NIN_BALLOONUSERCLICK || event == NIN_BALLOONTIMEOUT || event == NIN_BALLOONHIDE) {
            showing[0] = 0;
            if (!anyWaiting()) removeIcon();
        }
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

// The hidden window that owns the timers and the icon's messages, made on first use.
static HWND ensureOwner(void) {
    if (owner) return owner;
    WNDCLASSW type;
    memset(&type, 0, sizeof type);
    type.lpfnWndProc = ownerProc;
    type.hInstance = GetModuleHandleW(NULL);
    type.lpszClassName = L"EskiuNotifications";
    RegisterClassW(&type);
    owner = CreateWindowExW(0, type.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, type.hInstance, NULL);
    return owner;
}

// "denied" when the user turned notifications off for every app (Settings, System,
// Notifications), else "allowed".
static const char *permission(void) {
    DWORD value = 1, size = sizeof value;
    LONG status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",
                               L"ToastEnabled", RRF_RT_REG_DWORD, NULL, &value, &size);
    return status == ERROR_SUCCESS && value == 0 ? "permission denied" : "permission allowed";
}

// The answer comes after the call, as on the other platforms.
static void CALLBACK answer(HWND window, UINT message, UINT_PTR timer, DWORD time) {
    (void)message;
    (void)time;
    KillTimer(window, timer);
    ui_native_emit("notifications", permission());
}

void notifications_check(void) {
    if (ensureOwner()) SetTimer(owner, 0x7fff0000, 0, answer);
    else ui_native_emit("notifications", permission());
}

void notifications_request(void) { notifications_check(); }

static Scheduled *find(const char *id) {
    for (int i = 0; i < kMaxScheduled; i++) {
        if (waiting[i].timer && strcmp(waiting[i].id, id) == 0) return &waiting[i];
    }
    return NULL;
}

void notifications_native_cancel(const char *id) {
    Scheduled *s = find(id);
    if (s) {
        KillTimer(owner, s->timer);
        s->timer = 0;
    }
    if (showing[0] && strcmp(showing, id) == 0) {
        // Take the balloon away: an empty one hides it.
        NOTIFYICONDATAW data;
        iconData(&data);
        data.uFlags = NIF_INFO;
        Shell_NotifyIconW(NIM_MODIFY, &data);
        showing[0] = 0;
    }
    if (!showing[0] && !anyWaiting()) removeIcon();
}

void notifications_native_schedule(const char *id, const char *title, const char *body, int seconds) {
    if (!ensureOwner()) return;
    notifications_native_cancel(id);
    Scheduled *s = NULL;
    for (int i = 0; i < kMaxScheduled && !s; i++) {
        if (!waiting[i].timer) s = &waiting[i];
    }
    if (!s) return;
    snprintf(s->id, sizeof s->id, "%s", id);
    widen(title, s->title, sizeof s->title / sizeof s->title[0]);
    widen(body, s->body, sizeof s->body / sizeof s->body[0]);
    s->timer = SetTimer(owner, nextTimer++, (UINT)(seconds < 1 ? 1 : seconds) * 1000, NULL);
    if (nextTimer >= 0x7fff0000) nextTimer = 1;
}

const char *notifications_take_opened(void) {
    static char taken[128];
    snprintf(taken, sizeof taken, "%s", opened);
    opened[0] = 0;
    return taken;
}
