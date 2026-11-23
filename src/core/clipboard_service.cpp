#include "clipboard_service.h"

#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QPixmap>
#include <QVariant>

#ifdef Q_OS_ANDROID
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
#endif

ClipboardService &ClipboardService::instance()
{
    static ClipboardService inst;
    return inst;
}

ClipboardService::ClipboardService(QObject *parent)
    : QObject(parent)
{
}

void ClipboardService::copyFile(const QUrl &url)
{
#ifdef Q_OS_ANDROID
    // Android: put the content URI on the system clipboard. ClipData.newUri()
    // keeps the document URI (no byte copying), and the clipboard service
    // grants temporary read permission to the pasting app.
    if (url.scheme() != QLatin1String("content"))
        return;

    jni::callStaticBool(jni::CLIPBOARD_BRIDGE_CLASS, "copyDocument", url,
                        documentLabel(url));
#else
    // Desktop payload.
    if (!url.isLocalFile())
        return;

    const QString path = url.toLocalFile();
    QMimeData *mime = new QMimeData;

    mime->setData("application/x-qt-windows-mime;value=\"FileNameW\"",
                  QFileInfo(path).fileName().toUtf8());

    QImageReader reader(path);
    const QImage image = reader.read();
    if (!image.isNull()) {
        mime->setImageData(QVariant::fromValue(
            QPixmap::fromImage(image)));
    }

    QList<QUrl> urls;
    urls.append(url);
    mime->setUrls(urls);

    QGuiApplication::clipboard()->setMimeData(mime);
#endif
}

void ClipboardService::shareFile(const QUrl &url)
{
    Q_UNUSED(url);
#ifdef Q_OS_ANDROID
    // Android: hand the document URI to the system share sheet. The read
    // permission grant travels with the intent, so the receiving app loads
    // the GIF from the source provider.
    if (url.scheme() != QLatin1String("content"))
        return;

    jni::callStaticBool(jni::CLIPBOARD_BRIDGE_CLASS, "shareDocument", url,
                        documentLabel(url));
#endif
}
