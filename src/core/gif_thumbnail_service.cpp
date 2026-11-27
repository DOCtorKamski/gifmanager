#include "gif_thumbnail_service.h"

#include <QFutureWatcher>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace {

constexpr int MAX_LAZY_THUMBNAIL_THREADS = 4;

QImage makeLazyThumbnail(const QImage &image, const QSize &size)
{
    if (image.isNull()) {
        QImage placeholder(size, QImage::Format_RGB32);
        placeholder.fill(Qt::gray);
        return placeholder;
    }

    return image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace

GifThumbnailService::GifThumbnailService(std::shared_ptr<GifStorage> storage_,
                                         QObject *parent)
    : QObject(parent)
    , storage(std::move(storage_))
{
    pool.setMaxThreadCount(std::max(
        1, std::min(MAX_LAZY_THUMBNAIL_THREADS, QThread::idealThreadCount())));
}

void GifThumbnailService::setThumbnailSize(const QSize &size)
{
    if (thumbnail_size == size)
        return;

    thumbnail_size = size;
    clear();
}

void GifThumbnailService::request(const QUrl &url)
{
    if (!storage || url.isEmpty() || !thumbnail_size.isValid())
        return;
    if (pending.contains(url))
        return;

    pending.insert(url);
    startRequest(url, generation);
}

void GifThumbnailService::clear()
{
    ++generation;
    pending.clear();
}

void GifThumbnailService::startRequest(const QUrl &url, quint64 request_generation)
{
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this,
            [this, watcher, url, request_generation] {
        const QImage image = watcher->result();
        watcher->deleteLater();

        // A reload or thumbnail-size change invalidates requests that were
        // started before it; their results must not overwrite the new model.
        if (request_generation != generation)
            return;

        pending.remove(url);
        emit thumbnailReady(url, image);
    });

    const std::shared_ptr<GifStorage> storage_copy = storage;
    const QSize size = thumbnail_size;
    watcher->setFuture(QtConcurrent::run(&pool, [storage_copy, url, size] {
        return makeLazyThumbnail(storage_copy->readThumbnail(url, size), size);
    }));
}
