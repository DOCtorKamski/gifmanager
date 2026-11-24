#include "clipboard_service.h"
#include "clipboard_service_desktop.h"

#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QPixmap>
#include <QVariant>

QMimeData *createDesktopClipboardMimeData(const QUrl &url)
{
    if (!url.isLocalFile())
        return nullptr;

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

    return mime;
}

void ClipboardService::copyFile(const QUrl &url)
{
    if (QMimeData *mime = createDesktopClipboardMimeData(url))
        QGuiApplication::clipboard()->setMimeData(mime);
}

void ClipboardService::shareFile(const QUrl &url)
{
    // Desktop has no share sheet; the Share menu item is Android-only.
    Q_UNUSED(url);
}
