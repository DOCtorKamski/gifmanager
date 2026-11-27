#include "gif_loader_worker.h"

#include <QMetaType>
#include <QMutex>
#include <QQueue>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QWaitCondition>

#include <algorithm>
#include <utility>

namespace {

constexpr int MAX_THUMBNAIL_THREADS = 4;

struct ThumbnailResult
{
    QUrl url;
    QImage image;
};

struct ThumbnailBatch
{
    QMutex mutex;
    QWaitCondition ready;
    QQueue<ThumbnailResult> results;
    int finished = 0;
    int total = 0;
};

QImage makeThumbnail(const QImage &image, const QSize &size)
{
    if (image.isNull()) {
        QImage placeholder(size, QImage::Format_RGB32);
        placeholder.fill(Qt::gray);
        return placeholder;
    }

    return image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

class ThumbnailJob : public QRunnable
{
public:
    ThumbnailJob(std::shared_ptr<GifStorage> storage, GifItem item, QSize size,
                 std::shared_ptr<ThumbnailBatch> batch,
                 std::shared_ptr<QAtomicInt> stop)
        : storage(std::move(storage))
        , item(std::move(item))
        , size(size)
        , batch(std::move(batch))
        , stop(std::move(stop))
    {
    }

    void run() override
    {
        if (stop->loadAcquire() != 0) {
            finish();
            return;
        }

        const QImage image = storage->readThumbnail(item.url, size);
        if (stop->loadAcquire() != 0) {
            finish();
            return;
        }

        ThumbnailResult result;
        result.url = item.url;
        result.image = makeThumbnail(image, size);

        QMutexLocker locker(&batch->mutex);
        batch->results.enqueue(result);
        ++batch->finished;
        batch->ready.wakeOne();
    }

private:
    void finish()
    {
        QMutexLocker locker(&batch->mutex);
        ++batch->finished;
        batch->ready.wakeOne();
    }

    std::shared_ptr<GifStorage> storage;
    GifItem item;
    QSize size;
    std::shared_ptr<ThumbnailBatch> batch;
    std::shared_ptr<QAtomicInt> stop;
};

QThreadPool *thumbnailPool()
{
    // One process-wide pool: a cancelled load may leave a job running, and the
    // pool must outlive the loader thread that started it.
    static QThreadPool pool;
    pool.setMaxThreadCount(
        std::max(1, std::min(MAX_THUMBNAIL_THREADS, QThread::idealThreadCount())));
    return &pool;
}

} // namespace

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
    should_stop->storeRelease(1);
}

bool GifLoaderWorker::shouldStop() const
{
    return should_stop->loadAcquire() != 0;
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

    if (!request.load_thumbnails) {
        emit finished();
        return;
    }

    const int total = files.size();
    if (total == 0 || shouldStop()) {
        emit finished();
        return;
    }

    auto batch = std::make_shared<ThumbnailBatch>();
    QThreadPool *pool = thumbnailPool();

    int submitted = 0;
    for (const GifItem &item : files) {
        if (shouldStop())
            break;

        pool->start(new ThumbnailJob(request.storage, item,
                                     request.thumbnail_size, batch,
                                     should_stop));
        ++submitted;
    }
    batch->total = submitted;

    int emitted = 0;
    while (emitted < total && !shouldStop()) {
        ThumbnailResult result;
        bool has_result = false;

        {
            QMutexLocker locker(&batch->mutex);
            while (batch->results.isEmpty() && batch->finished < batch->total
                   && !shouldStop()) {
                batch->ready.wait(&batch->mutex, 50);
            }

            if (!batch->results.isEmpty()) {
                result = batch->results.dequeue();
                has_result = true;
            } else if (batch->finished >= batch->total) {
                break;
            }
        }

        if (!has_result)
            continue;

        if (shouldStop())
            break;

        emit gifLoaded(result.url, result.image);
        emit progress(++emitted, total);
    }

    // Normal completion means every submitted job has already reported. On
    // cancellation the shared batch and stop flag keep any in-flight jobs safe,
    // so there is no need to block this thread waiting for them.
    emit finished();
}
