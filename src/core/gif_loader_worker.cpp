#include "gif_loader_worker.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

GifLoaderWorker::GifLoaderWorker(const QStringList &files,
                                 const QSize &thumb_size,
                                 quint64 load_id)
    : file_paths(files),
      thumbnail_size(thumb_size),
      load_id(load_id),
      should_stop(0)
{
}

GifLoaderWorker::GifLoaderWorker(const QString &folder_path,
                                 const QSize &thumb_size,
                                 quint64 load_id)
    : thumbnail_size(thumb_size),
      load_id(load_id),
      should_stop(0),
      scan_mode(true),
      scan_folder(folder_path)
{
}

void GifLoaderWorker::requestStop() {
    should_stop.storeRelease(1);
    qDebug() << "Stop requested for load_id:" << load_id;
}

void GifLoaderWorker::process() {
    QStringList files = file_paths;

    if (scan_mode) {
        files = scanFolder();
        if (should_stop.loadAcquire()) {
            qDebug() << "Worker stopped while scanning, load_id:" << load_id;
            emit finished(load_id);
            return;
        }

        qDebug() << "Folder scanned, files count:" << files.count()
                 << "load_id:" << load_id;
        emit filesDiscovered(load_id, files);
    }

    qDebug() << "Worker started, files count:" << files.count() << "load_id:" << load_id;

    int total = files.count();
    int current = 0;

    for (const QString &file_path : files) {
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

QStringList GifLoaderWorker::scanFolder()
{
    QStringList files;

    QDirIterator iterator(scan_folder, {"*.gif", "*.GIF"}, QDir::Files);
    while (iterator.hasNext()) {
        if (should_stop.loadAcquire())
            return files;

        iterator.next();
        files.append(iterator.fileInfo().absoluteFilePath());
    }

    std::sort(files.begin(), files.end(),
              [](const QString &lhs, const QString &rhs) {
                  return QFileInfo(lhs).fileName() < QFileInfo(rhs).fileName();
              });

    return files;
}
