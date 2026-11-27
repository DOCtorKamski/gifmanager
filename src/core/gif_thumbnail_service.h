#ifndef GIF_THUMBNAIL_SERVICE_H
#define GIF_THUMBNAIL_SERVICE_H

#include "gif_storage.h"

#include <QImage>
#include <QObject>
#include <QSet>
#include <QSize>
#include <QThreadPool>
#include <QUrl>

#include <memory>

// Loads grid thumbnails on demand. The grid requests only the cells that are
// currently visible, so opening a large folder no longer decodes every GIF
// before the view becomes usable.
class GifThumbnailService : public QObject
{
    Q_OBJECT
public:
    explicit GifThumbnailService(std::shared_ptr<GifStorage> storage,
                                 QObject *parent = nullptr);

    void setThumbnailSize(const QSize &size);
    void request(const QUrl &url);
    void clear();

signals:
    void thumbnailReady(const QUrl &url, const QImage &image);

private:
    void startRequest(const QUrl &url, quint64 generation);

    std::shared_ptr<GifStorage> storage;
    QSize thumbnail_size;
    QSet<QUrl> pending;
    quint64 generation = 0;
    QThreadPool pool;
};

#endif // GIF_THUMBNAIL_SERVICE_H
