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
    void gifLoaded(const QUrl &url, const QImage &thumbnail);
    void progress(int current, int total);
    void finished();

private:
    bool shouldStop() const;

    GifLoadRequest request;
    QAtomicInt should_stop{0};
};

#endif // GIF_LOADER_WORKER_H
