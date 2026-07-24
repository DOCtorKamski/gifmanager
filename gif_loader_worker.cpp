#include "gif_loader_worker.h"
#include <QImageReader>
#include <QDebug>

GifLoaderWorker::GifLoaderWorker(const QStringList &files,
                                 const QSize &thumb_size,
                                 quint64 load_id)
    : file_paths(files),
    thumbnail_size(thumb_size),
    load_id(load_id),
    should_stop(0)
{
}

void GifLoaderWorker::requestStop() {
    should_stop.storeRelease(1);
    qDebug() << "Stop requested for load_id:" << load_id;
}

void GifLoaderWorker::process() {
    qDebug() << "Worker started, files count:" << file_paths.count() << "load_id:" << load_id;

    int total = file_paths.count();
    int current = 0;

    for (const QString &file_path : file_paths) {
        if (should_stop.loadAcquire()) {
            qDebug() << "Worker stopped at" << current << "/" << total << "load_id:" << load_id;
            emit finished(load_id);
            return;
        }

        QImageReader reader(file_path);
        reader.setScaledSize(thumbnail_size);
        // double scale need here, if dont looks ugly
        QPixmap thumbnail;
        QImage img = reader.read();
        if (!img.isNull()) {
            thumbnail = QPixmap::fromImage(
                img.scaled(thumbnail_size, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation)
                );
        } else {
            thumbnail = QPixmap(thumbnail_size);
            thumbnail.fill(Qt::gray);
        }

        LoadedGifData data;
        data.file_path = file_path;
        data.thumbnail = thumbnail;

        if (!should_stop.loadAcquire()) {
            emit gifLoaded(load_id, data);
            emit progress(++current, total);
        } else {
            qDebug() << "Stopped before emit at" << current << "load_id:" << load_id;
            emit finished(load_id);
            return;
        }
    }

    qDebug() << "Worker finished normally, processed:" << current << "/" << total << "load_id:" << load_id;
    emit finished(load_id);
}
