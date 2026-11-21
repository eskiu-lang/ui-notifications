// <notifications> on Linux: the desktop's notification server (org.freedesktop.Notifications,
// which GNOME, KDE and the other desktops run) over D-Bus through GIO, which GTK already links.
// A scheduled notification is a timer in the app, so it shows only while the app runs, as on
// the web. A click on it (the default action) brings the window up and tells the app. There
// is no permission to ask for: the app may notify when a server answers on the session bus,
// and is "denied" when none does (a session without one, a server switched off).
#include <stdio.h>
#include <string.h>
#include "ui_native.h"

static const char *kName = "org.freedesktop.Notifications";
static const char *kPath = "/org/freedesktop/Notifications";

// One notification the app scheduled: its id, what it says, its timer while it waits and the
// server's id once it shows.
typedef struct {
    char *id, *title, *body;
    guint timer;
    guint32 shown;
} Scheduled;

static GDBusConnection *bus;
static GHashTable *scheduled;   // id -> Scheduled
static char *opened;            // the id of a notification the user opened, until taken

static void dropScheduled(gpointer data) {
    Scheduled *s = data;
    if (s->timer) g_source_remove(s->timer);
    g_free(s->id);
    g_free(s->title);
    g_free(s->body);
    g_free(s);
}

static Scheduled *byServerId(guint32 shown) {
    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, scheduled);
    while (g_hash_table_iter_next(&it, &key, &value)) {
        if (((Scheduled *)value)->shown == shown) return value;
    }
    return NULL;
}

static void onSignal(GDBusConnection *connection, const char *sender, const char *path, const char *interface,
                     const char *signal, GVariant *parameters, gpointer data) {
    (void)connection; (void)sender; (void)path; (void)interface; (void)data;
    guint32 shown = 0;
    if (!strcmp(signal, "ActionInvoked")) {
        const char *action = NULL;
        g_variant_get(parameters, "(u&s)", &shown, &action);
        Scheduled *s = byServerId(shown);
        if (!s || strcmp(action, "default") != 0) return;
        g_free(opened);
        opened = g_strdup(s->id);
        GtkWindow *window = ui_native_window();
        if (window) gtk_window_present(window);
        char *payload = g_strdup_printf("opened %s", s->id);
        ui_native_emit("notifications", payload);
        g_free(payload);
    } else if (!strcmp(signal, "NotificationClosed")) {
        guint32 reason = 0;
        g_variant_get(parameters, "(uu)", &shown, &reason);
        Scheduled *s = byServerId(shown);
        if (s) g_hash_table_remove(scheduled, s->id);
    }
}

// The session bus and the server's signals, once; NULL when there is no session bus.
static GDBusConnection *busConnect(void) {
    if (bus) return bus;
    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!bus) return NULL;
    scheduled = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, dropScheduled);
    g_dbus_connection_signal_subscribe(bus, kName, kName, NULL, kPath, NULL, G_DBUS_SIGNAL_FLAGS_NONE, onSignal, NULL, NULL);
    return bus;
}

static void answered(GObject *source, GAsyncResult *result, gpointer data) {
    (void)data;
    GVariant *reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, NULL);
    ui_native_emit("notifications", reply ? "permission allowed" : "permission denied");
    if (reply) g_variant_unref(reply);
}

// Whether a notification server answers: GetServerInformation, which also starts one that
// D-Bus activates.
void notifications_check(void) {
    if (!busConnect()) { ui_native_emit("notifications", "permission denied"); return; }
    g_dbus_connection_call(bus, kName, kPath, kName, "GetServerInformation", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, 5000,
                           NULL, answered, NULL);
}

void notifications_request(void) { notifications_check(); }

static void shownAs(GObject *source, GAsyncResult *result, gpointer data) {
    char *id = data;
    GVariant *reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, NULL);
    Scheduled *s = id ? g_hash_table_lookup(scheduled, id) : NULL;
    if (reply && s) g_variant_get(reply, "(u)", &s->shown);
    if (reply) g_variant_unref(reply);
    g_free(id);
}

static gboolean show(gpointer data) {
    Scheduled *s = data;
    s->timer = 0;
    GVariantBuilder actions;
    g_variant_builder_init(&actions, G_VARIANT_TYPE("as"));
    g_variant_builder_add(&actions, "s", "default");
    g_variant_builder_add(&actions, "s", "Open");
    GVariantBuilder hints;
    g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
    const char *app = g_get_prgname();
    if (app) g_variant_builder_add(&hints, "{sv}", "desktop-entry", g_variant_new_string(app));
    const char *name = g_get_application_name();
    g_dbus_connection_call(bus, kName, kPath, kName, "Notify",
                           g_variant_new("(susssasa{sv}i)", name ? name : "", s->shown, "", s->title, s->body, &actions, &hints, -1),
                           G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, 5000, NULL, shownAs, g_strdup(s->id));
    return G_SOURCE_REMOVE;
}

static void closeShown(guint32 shown) {
    if (!shown) return;
    g_dbus_connection_call(bus, kName, kPath, kName, "CloseNotification", g_variant_new("(u)", shown), NULL,
                           G_DBUS_CALL_FLAGS_NONE, 5000, NULL, NULL, NULL);
}

void notifications_native_schedule(const char *id, const char *title, const char *body, int seconds) {
    if (!busConnect()) return;
    Scheduled *s = g_hash_table_lookup(scheduled, id);
    guint32 replaces = 0;
    if (s) {
        // The same id replaces it: the server swaps the shown one in place.
        replaces = s->shown;
        g_hash_table_remove(scheduled, id);
    }
    s = g_new0(Scheduled, 1);
    s->id = g_strdup(id);
    s->title = g_strdup(title);
    s->body = g_strdup(body);
    s->shown = replaces;
    g_hash_table_insert(scheduled, s->id, s);
    s->timer = g_timeout_add_seconds((guint)(seconds < 1 ? 1 : seconds), show, s);
}

void notifications_native_cancel(const char *id) {
    if (!busConnect()) return;
    Scheduled *s = g_hash_table_lookup(scheduled, id);
    if (!s) return;
    closeShown(s->shown);
    g_hash_table_remove(scheduled, id);
}

const char *notifications_take_opened(void) {
    static char taken[256];
    taken[0] = 0;
    if (opened) {
        snprintf(taken, sizeof taken, "%s", opened);
        g_free(opened);
        opened = NULL;
    }
    return taken;
}
