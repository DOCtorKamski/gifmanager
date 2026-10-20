#ifndef GIF_LOADER_WORKER_H
#define GIF_LOADER_WORKER_H

#include <QAtomicInt>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QThread>

class GifLoaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit GifLoaderWorker(const QStringList &files,
                             const QSize &thumb_size,
                             quint64 load_id = 0);
    explicit GifLoaderWorker(const QString &folder_path,
                             const QSize &thumb_size,
                             quint64 load_id = 0);

    void requestStop();

public slots:
    void process();

signals:
    void filesDiscovered(quint64 load_id, const QStringList &file_paths);
    void gifLoaded(quint64 load_id, const QString &file_path, const QImage &thumbnail);
    void finished(quint64 load_id);
    void progress(quint64 load_id, int current, int total);

private:
    QStringList scanFolder();

    QStringList file_paths;
    QSize thumbnail_size;
    quint64 load_id;
    QAtomicInt should_stop;
    bool scan_mode = false;
    QString scan_folder;
};

#endif // GIF_LOADER_WORKER_H
