#include "gif_loader_worker.h"

#include <QMetaType>

#include <utility>

GifLoadRequest GifLoadRequest::fromContainer(const QUrl &container,
                                             const QSize &thumbnail_size,
                                             std::shared_ptr<GifStorage> storage)
{
    GifLoadRequest request;
    request.source = Source::Container;
    request.container = container;
    request.thumbnail_size = thumbnail_size;
    request.storage = std::move(storage);
    return request;
}

GifLoadRequest GifLoadRequest::fromFiles(const QList<GifItem> &files,
                                         const QSize &thumbnail_size,
                                         std::shared_ptr<GifStorage> storage)
{
    GifLoadRequest request;
    request.source = Source::FileList;
    request.files = files;
    request.thumbnail_size = thumbnail_size;
    request.storage = std::move(storage);
    return request;
}

GifLoaderWorker::GifLoaderWorker(GifLoadRequest request_)
    : request(std::move(request_))
{
    // The list is forwarded across the loader thread and captured by QSignalSpy
    // in tests, so make the metatypes available before any connection is made.
    qRegisterMetaType<GifItem>("GifItem");
    qRegisterMetaType<QList<GifItem>>("QList<GifItem>");
}

void GifLoaderWorker::requestStop()
{
    should_stop.storeRelease(1);
}

bool GifLoaderWorker::shouldStop() const
{
    return should_stop.loadAcquire() != 0;
}

void GifLoaderWorker::process()
{
    if (!request.storage) {
        emit finished();
        return;
    }

    QList<GifItem> files = request.files;

    if (request.source == GifLoadRequest::Source::Container) {
        const GifStorage::Listing listing = request.storage->listGifs(
            request.container, [this] { return shouldStop(); });
        if (shouldStop()) {
            emit finished();
            return;
        }

        if (listing.failed()) {
            // The folder exists (the library checks that before starting) but
            // cannot be listed, e.g. revoked SAF permission. Report it so the
            // UI does not silently show an empty grid.
            emit discoveryFailed(listing.error);
            emit finished();
            return;
        }

        files = listing.items;
        emit filesDiscovered(files);
    }

    const int total = files.size();
    int current = 0;

    for (const GifItem &item : files) {
        if (shouldStop())
            break;

        QImage image = request.storage->readThumbnail(item.url,
                                                      request.thumbnail_size);
        QImage thumbnail;
        if (!image.isNull()) {
            thumbnail = image.scaled(request.thumbnail_size, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
        } else {
            thumbnail = QImage(request.thumbnail_size, QImage::Format_RGB32);
            thumbnail.fill(Qt::gray);
        }

        if (shouldStop())
            break;

        emit gifLoaded(item.url, thumbnail);
        emit progress(++current, total);
    }

    emit finished();
}
