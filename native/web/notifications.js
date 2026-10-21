// <notifications> on the web: the Notification API, as an emscripten library defining the
// externs. A scheduled notification is a timer in the page, so it shows only while the page is
// open; a click on it focuses the page and tells the app.
mergeInto(LibraryManager.library, {
  notifications_check: function() {
    const state = !('Notification' in window) ? 'denied'
      : Notification.permission === 'granted' ? 'allowed' : Notification.permission === 'denied' ? 'denied' : 'notasked';
    setTimeout(() => Module.uiEmit('notifications', 'permission ' + state), 0);
  },
  notifications_request: function() {
    if (!('Notification' in window)) { Module.uiEmit('notifications', 'permission denied'); return; }
    Notification.requestPermission().then(result => {
      Module.uiEmit('notifications', 'permission ' + (result === 'granted' ? 'allowed' : result === 'denied' ? 'denied' : 'notasked'));
    });
  },
  notifications_native_schedule__deps: ['$UTF8ToString'],
  notifications_native_schedule: function(id, title, body, seconds) {
    const key = UTF8ToString(id), text = UTF8ToString(title), detail = UTF8ToString(body);
    if (!Module.notificationTimers) Module.notificationTimers = {};
    clearTimeout(Module.notificationTimers[key]);
    Module.notificationTimers[key] = setTimeout(() => {
      delete Module.notificationTimers[key];
      if (!('Notification' in window) || Notification.permission !== 'granted') return;
      const shown = new Notification(text, {body: detail, tag: key});
      if (!Module.notificationsShown) Module.notificationsShown = {};
      Module.notificationsShown[key] = shown;
      shown.onclick = () => { window.focus(); shown.close(); Module.uiEmit('notifications', 'opened ' + key); };
    }, seconds * 1000);
  },
  notifications_native_cancel__deps: ['$UTF8ToString'],
  notifications_native_cancel: function(id) {
    const key = UTF8ToString(id);
    if (Module.notificationTimers) { clearTimeout(Module.notificationTimers[key]); delete Module.notificationTimers[key]; }
    if (Module.notificationsShown && Module.notificationsShown[key]) { Module.notificationsShown[key].close(); delete Module.notificationsShown[key]; }
  },
  notifications_take_opened__deps: ['$stringToNewUTF8'],
  notifications_take_opened: function() {
    if (!Module.notificationsEmpty) Module.notificationsEmpty = stringToNewUTF8('');
    return Module.notificationsEmpty;
  },
});
