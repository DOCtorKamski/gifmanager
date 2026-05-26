#ifndef GIF_LOADER_WORKER_H
#define GIF_LOADER_WORKER_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <QAtomicInt>

struct LoadedGifData {
    QString file_path;
    QPixmap thumbnail;
};

class GifLoaderWorker : public QObject {
    Q_OBJECT
public:
    explicit GifLoaderWorker(const QStringList &files,
                             const QSize &thumb_size,
                             const QString &search_query = "",
                             quint64 load_id = 0);

    void requestStop();

public slots:
    void process();

signals:
    void gifLoaded(quint64 load_id, const LoadedGifData &data);
    void finished(quint64 load_id);
    void progress(int current, int total);

private:
    QStringList file_paths;
    QSize thumbnail_size;
    QString query;
    quint64 load_id;
    QAtomicInt should_stop;
};

#endif // GIF_LOADER_WORKER_H
