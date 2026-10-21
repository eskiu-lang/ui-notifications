// <notifications> on iOS: UNUserNotificationCenter. Its delegate is set before the app
// finishes launching, so a notification that opens the app reaches it; one that arrives while
// the app is in front shows as a banner too.
#import <UserNotifications/UserNotifications.h>
#import "ui_native.h"

@interface EskiuNotifications : NSObject <UNUserNotificationCenterDelegate>
@end

static EskiuNotifications *shared;
// The id of a notification that opened the app, until the app takes it.
static NSString *opened;

static void emitPermission(UNAuthorizationStatus status) {
    const char *value = status == UNAuthorizationStatusDenied ? "denied"
                      : status == UNAuthorizationStatusNotDetermined ? "notasked" : "allowed";
    char payload[64];
    snprintf(payload, sizeof payload, "permission %s", value);
    ui_native_emit("notifications", payload);
}

@implementation EskiuNotifications
- (void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))done {
    done(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList | UNNotificationPresentationOptionSound);
}
- (void)userNotificationCenter:(UNUserNotificationCenter *)center didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)(void))done {
    NSString *id = response.notification.request.identifier;
    dispatch_async(dispatch_get_main_queue(), ^{
        opened = id;
        NSString *payload = [@"opened " stringByAppendingString:id];
        ui_native_emit("notifications", payload.UTF8String);
    });
    done();
}
@end

UI_NATIVE_SETUP(notifications) {
    shared = [EskiuNotifications new];
    UNUserNotificationCenter.currentNotificationCenter.delegate = shared;
}

void notifications_check(void) {
    [UNUserNotificationCenter.currentNotificationCenter getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
        emitPermission(settings.authorizationStatus);
    }];
}

void notifications_request(void) {
    UNUserNotificationCenter *center = UNUserNotificationCenter.currentNotificationCenter;
    [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge
                          completionHandler:^(BOOL granted, NSError *error) {
        [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
            emitPermission(settings.authorizationStatus);
        }];
    }];
}

void notifications_native_schedule(const char *id, const char *title, const char *body, int seconds) {
    UNMutableNotificationContent *content = [UNMutableNotificationContent new];
    content.title = @(title);
    content.body = @(body);
    content.sound = UNNotificationSound.defaultSound;
    UNTimeIntervalNotificationTrigger *trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:seconds repeats:NO];
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@(id) content:content trigger:trigger];
    [UNUserNotificationCenter.currentNotificationCenter addNotificationRequest:request withCompletionHandler:nil];
}

void notifications_native_cancel(const char *id) {
    NSArray *ids = @[@(id)];
    UNUserNotificationCenter *center = UNUserNotificationCenter.currentNotificationCenter;
    [center removePendingNotificationRequestsWithIdentifiers:ids];
    [center removeDeliveredNotificationsWithIdentifiers:ids];
}

const char *notifications_take_opened(void) {
    static char taken[256];
    taken[0] = 0;
    if (opened) {
        snprintf(taken, sizeof taken, "%s", opened.UTF8String);
        opened = nil;
    }
    return taken;
}
