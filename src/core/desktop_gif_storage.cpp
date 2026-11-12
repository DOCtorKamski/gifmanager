#include "desktop_gif_storage.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

bool DesktopGifStorage::exists(const QUrl &container) const
{
    return container.isLocalFile() && QDir(container.toLocalFile()).exists();
}

QList<GifItem> DesktopGifStorage::listGifs(const QUrl &container,
                                           const StopPredicate &shouldStop)
{
    QList<GifItem> items;

    if (!container.isLocalFile())
        return items;

    QDirIterator iterator(container.toLocalFile(), {"*.gif", "*.GIF"}, QDir::Files);
    while (iterator.hasNext()) {
        if (shouldStop && shouldStop())
            break;

        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        items.append({QUrl::fromLocalFile(info.absoluteFilePath()), info.fileName()});
    }

    std::sort(items.begin(), items.end(),
              [](const GifItem &lhs, const GifItem &rhs) {
                  return lhs.fileName < rhs.fileName;
              });

    return items;
}

QImage DesktopGifStorage::readThumbnail(const QUrl &url, const QSize &size)
{
    if (!url.isLocalFile())
        return {};

    QImageReader reader(url.toLocalFile());
    reader.setScaledSize(size);

    const QImage image = reader.read();
    if (image.isNull())
        return {};

    return image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
