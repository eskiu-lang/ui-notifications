// <notifications> on Android: the externs Eskiu calls, into EskiuNotifications (Java) through JNI.
#include <jni.h>
#include <string.h>
#include "ui_native.h"

static jclass notifications(JNIEnv *env) { return ui_find_class("org.eskiu.notifications.EskiuNotifications"); }

static void call(const char *method) {
    JNIEnv *env = ui_jni_env();
    jclass type = notifications(env);
    (*env)->CallStaticVoidMethod(env, type, (*env)->GetStaticMethodID(env, type, method, "()V"));
    (*env)->DeleteLocalRef(env, type);
}

void notifications_check(void) { call("check"); }

void notifications_request(void) { call("request"); }

void notifications_native_schedule(const char *id, const char *title, const char *body, int seconds) {
    JNIEnv *env = ui_jni_env();
    jclass type = notifications(env);
    jmethodID method = (*env)->GetStaticMethodID(env, type, "schedule", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    jstring a = (*env)->NewStringUTF(env, id), b = (*env)->NewStringUTF(env, title), c = (*env)->NewStringUTF(env, body);
    (*env)->CallStaticVoidMethod(env, type, method, a, b, c, (jint)seconds);
    (*env)->DeleteLocalRef(env, a); (*env)->DeleteLocalRef(env, b); (*env)->DeleteLocalRef(env, c);
    (*env)->DeleteLocalRef(env, type);
}

void notifications_native_cancel(const char *id) {
    JNIEnv *env = ui_jni_env();
    jclass type = notifications(env);
    jmethodID method = (*env)->GetStaticMethodID(env, type, "cancel", "(Ljava/lang/String;)V");
    jstring a = (*env)->NewStringUTF(env, id);
    (*env)->CallStaticVoidMethod(env, type, method, a);
    (*env)->DeleteLocalRef(env, a);
    (*env)->DeleteLocalRef(env, type);
}

const char *notifications_take_opened(void) {
    static char taken[256];
    JNIEnv *env = ui_jni_env();
    jclass type = notifications(env);
    jstring id = (jstring)(*env)->CallStaticObjectMethod(env, type,
        (*env)->GetStaticMethodID(env, type, "takeOpened", "()Ljava/lang/String;"));
    const char *chars = (*env)->GetStringUTFChars(env, id, NULL);
    strncpy(taken, chars, sizeof taken - 1);
    taken[sizeof taken - 1] = 0;
    (*env)->ReleaseStringUTFChars(env, id, chars);
    (*env)->DeleteLocalRef(env, id);
    (*env)->DeleteLocalRef(env, type);
    return taken;
}
