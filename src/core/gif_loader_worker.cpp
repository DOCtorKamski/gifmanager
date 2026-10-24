#include "gif_loader_worker.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>
#include <utility>

GifLoadRequest GifLoadRequest::fromFolder(const QString &folder_path,
                                          const QSize &thumbnail_size)
{
    GifLoadRequest request;
    request.source = Source::Folder;
    request.folder = folder_path;
    request.thumbnail_size = thumbnail_size;
    return request;
}

GifLoadRequest GifLoadRequest::fromFiles(const QStringList &file_paths,
                                         const QSize &thumbnail_size)
{
    GifLoadRequest request;
    request.source = Source::FileList;
    request.files = file_paths;
    request.thumbnail_size = thumbnail_size;
    return request;
}

GifLoaderWorker::GifLoaderWorker(GifLoadRequest request_)
    : request(std::move(request_))
{
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
    QStringList files = request.files;

    if (request.source == GifLoadRequest::Source::Folder) {
        files = scanFolder();
        if (shouldStop()) {
            emit finished();
            return;
        }

        emit filesDiscovered(files);
    }

    const int total = files.size();
    int current = 0;

    for (const QString &file_path : files) {
        if (shouldStop())
            break;

        QImageReader reader(file_path);
        reader.setScaledSize(request.thumbnail_size);

        QImage image = reader.read();
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

        emit gifLoaded(file_path, thumbnail);
        emit progress(++current, total);
    }

    emit finished();
}

QStringList GifLoaderWorker::scanFolder()
{
    QStringList files;

    QDirIterator iterator(request.folder, {"*.gif", "*.GIF"}, QDir::Files);
    while (iterator.hasNext()) {
        if (shouldStop())
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
