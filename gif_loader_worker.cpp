#include "gif_loader_worker.h"

#include <QDebug>
#include <QImageReader>

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

        QImage image = reader.read();
        QImage thumbnail;
        if (!image.isNull()) {
            thumbnail = image.scaled(thumbnail_size, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
        } else {
            thumbnail = QImage(thumbnail_size, QImage::Format_RGB32);
            thumbnail.fill(Qt::gray);
        }

        if (!should_stop.loadAcquire()) {
            emit gifLoaded(load_id, file_path, thumbnail);
            emit progress(load_id, ++current, total);
        } else {
            qDebug() << "Stopped before emit at" << current << "load_id:" << load_id;
            emit finished(load_id);
            return;
        }
    }

    qDebug() << "Worker finished normally, processed:" << current << "/" << total
             << "load_id:" << load_id;
    emit finished(load_id);
}
