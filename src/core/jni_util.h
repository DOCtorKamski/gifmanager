#ifndef JNI_UTIL_H
#define JNI_UTIL_H

// Shared JNI glue for the Android bridges. Only usable in an Android build: it
// includes QJniObject and the Android application context API. Both
// android_gif_storage.cpp and the Android branch of clipboard_service.cpp use
// the same context getter and string helpers, so they are centralized here
// instead of being repeated per call site.

#include <QCoreApplication>
#include <QJniObject>
#include <QString>
#include <QUrl>

namespace jni {

inline constexpr const char *SAF_STORAGE_CLASS =
    "org/qtproject/gifmanager/SafStorage";
inline constexpr const char *CLIPBOARD_BRIDGE_CLASS =
    "org/qtproject/gifmanager/ClipboardBridge";

// Activity context when available; every Java bridge method takes it as the
// first argument.
inline QJniObject androidContext()
{
    return QNativeInterface::QAndroidApplication::context();
}

// Null-safe QString extraction for JNI return values.
inline QString string(const QJniObject &value)
{
    return value.isValid() ? value.toString() : QString();
}

// Decoded last path segment of a document URL, suitable as a user-visible
// label. Percent-encoded document IDs would otherwise leak into menu titles.
inline QString documentLabel(const QUrl &url)
{
    return QUrl::fromPercentEncoding(url.fileName().toUtf8());
}

// static boolean method(Context, String)
inline bool callStaticBool(const char *className, const char *method,
                           const QUrl &url)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    return QJniObject::callStaticMethod<jboolean>(
        className, method, "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object(), uri.object());
}

// static boolean method(Context, String, String)
inline bool callStaticBool(const char *className, const char *method,
                           const QUrl &url, const QString &argument)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject arg = QJniObject::fromString(argument);
    return QJniObject::callStaticMethod<jboolean>(
        className, method,
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        context.object(), uri.object(), arg.object());
}

// static String method(Context, String)
inline QJniObject callStaticString(const char *className, const char *method,
                                   const QUrl &url)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    return QJniObject::callStaticObjectMethod(
        className, method,
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        context.object(), uri.object());
}

} // namespace jni

#endif // JNI_UTIL_H
