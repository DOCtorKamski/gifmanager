#ifndef THUMBNAIL_PROVIDER_H
#define THUMBNAIL_PROVIDER_H

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class ThumbnailProvider : public QQuickImageProvider
{
public:
    ThumbnailProvider();

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requested_size) override;

    void setImage(const QString &id, const QImage &image);
    void removeImage(const QString &id);
    void clear();

private:
    mutable QMutex mutex;
    QHash<QString, QImage> images;
};

#endif // THUMBNAIL_PROVIDER_H
