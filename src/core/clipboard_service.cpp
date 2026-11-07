#include "clipboard_service.h"

#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QPixmap>
#include <QUrl>
#include <QVariant>

ClipboardService &ClipboardService::instance()
{
    static ClipboardService inst;
    return inst;
}

ClipboardService::ClipboardService(QObject *parent)
    : QObject(parent)
{
}

void ClipboardService::copyFile(const QString &path)
{
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
    urls.append(QUrl::fromLocalFile(path));
    mime->setUrls(urls);

    QGuiApplication::clipboard()->setMimeData(mime);
}
