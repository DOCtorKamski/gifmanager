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
#include <QCoreApplication>
#include <QJniObject>

namespace {

constexpr const char *CLIPBOARD_BRIDGE_CLASS =
    "org/qtproject/gifmanager/ClipboardBridge";

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

    const QString name = QUrl::fromPercentEncoding(url.fileName().toUtf8());
    const QJniObject context =
        QNativeInterface::QAndroidApplication::context();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject label = QJniObject::fromString(
        name.isEmpty() ? QStringLiteral("GIF") : name);

    QJniObject::callStaticMethod<jboolean>(
        CLIPBOARD_BRIDGE_CLASS, "copyDocument",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        context.object(), uri.object(), label.object());
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
