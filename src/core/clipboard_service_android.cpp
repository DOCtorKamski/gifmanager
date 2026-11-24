#include "clipboard_service.h"

#include "jni_util.h"

namespace {

// User-visible label for the clip/intent; falls back to "GIF" because document
// URIs do not always end in a file name.
QString documentLabel(const QUrl &url)
{
    const QString name = jni::documentLabel(url);
    return name.isEmpty() ? QStringLiteral("GIF") : name;
}

} // namespace

void ClipboardService::copyFile(const QUrl &url)
{
    // Android: put the content URI on the system clipboard. ClipData.newUri()
    // keeps the document URI (no byte copying), and the clipboard service
    // grants temporary read permission to the pasting app.
    if (url.scheme() != QLatin1String("content"))
        return;

    jni::callStaticBool(jni::CLIPBOARD_BRIDGE_CLASS, "copyDocument", url,
                        documentLabel(url));
}

void ClipboardService::shareFile(const QUrl &url)
{
    // Android: hand the document URI to the system share sheet. The read
    // permission grant travels with the intent, so the receiving app loads
    // the GIF from the source provider.
    if (url.scheme() != QLatin1String("content"))
        return;

    jni::callStaticBool(jni::CLIPBOARD_BRIDGE_CLASS, "shareDocument", url,
                        documentLabel(url));
}
