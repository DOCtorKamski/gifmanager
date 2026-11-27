#ifndef GIF_LOADER_WORKER_H
#define GIF_LOADER_WORKER_H

#include "gif_item.h"
#include "gif_storage.h"

#include <QAtomicInt>
#include <QImage>
#include <QList>
#include <QObject>
#include <QSize>
#include <QUrl>

#include <memory>

struct GifLoadRequest
{
    enum class Source {
        Container,
        FileList
    };

    Source source = Source::FileList;
    QUrl container;
    QList<GifItem> files;
    QSize thumbnail_size;
    std::shared_ptr<GifStorage> storage;
    // When false the worker only discovers the files; thumbnails are loaded on
    // demand by GifThumbnailService. Tests and callers that need a complete
    // eager load keep the default.
    bool load_thumbnails = true;

    static GifLoadRequest fromContainer(const QUrl &container,
                                        const QSize &thumbnail_size,
                                        std::shared_ptr<GifStorage> storage);
    static GifLoadRequest fromFiles(const QList<GifItem> &files,
                                    const QSize &thumbnail_size,
                                    std::shared_ptr<GifStorage> storage);
};

class GifLoaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit GifLoaderWorker(GifLoadRequest request);

    void requestStop();

public slots:
    void process();

signals:
    void filesDiscovered(const QList<GifItem> &items);
    // Emitted instead of filesDiscovered() when the storage backend cannot read
    // the container; the worker then stops without generating thumbnails.
    void discoveryFailed(const QString &error);
    void gifLoaded(const QUrl &url, const QImage &thumbnail);
    void progress(int current, int total);
    void finished();

private:
    bool shouldStop() const;

    GifLoadRequest request;
    // Shared with the thumbnail jobs: a cancelled worker may be destroyed while
    // a pool thread is still finishing a read, so the flag must not live on the
    // worker itself.
    std::shared_ptr<QAtomicInt> should_stop = std::make_shared<QAtomicInt>(0);
};

#endif // GIF_LOADER_WORKER_H
