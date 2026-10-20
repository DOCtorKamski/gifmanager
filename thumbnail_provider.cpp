#include "thumbnail_provider.h"

#include <QMutexLocker>

ThumbnailProvider::ThumbnailProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ThumbnailProvider::requestImage(const QString &id, QSize *size,
                                       const QSize &requestedSize)
{
    QImage image;
    {
        QMutexLocker locker(&mutex);
        image = images.value(id);
    }

    if (image.isNull()) {
        if (size)
            *size = QSize();
        return image;
    }

    if (requestedSize.isValid() && requestedSize != image.size())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (size)
        *size = image.size();

    return image;
}

void ThumbnailProvider::setImage(const QString &id, const QImage &image)
{
    QMutexLocker locker(&mutex);
    images.insert(id, image);
}

void ThumbnailProvider::removeImage(const QString &id)
{
    QMutexLocker locker(&mutex);
    images.remove(id);
}

void ThumbnailProvider::clear()
{
    QMutexLocker locker(&mutex);
    images.clear();
}
