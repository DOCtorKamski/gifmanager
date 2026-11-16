#include "desktop_gif_storage.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>

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

QString DesktopGifStorage::validateFile(const QUrl &url) const
{
    if (!url.isLocalFile())
        return tr("Unsupported location.");
    if (!QFileInfo::exists(url.toLocalFile()))
        return tr("File does not exist.");
    return {};
}

GifStorage::RenamePlan DesktopGifStorage::planRename(
    const QUrl &url, const QString &new_base_name) const
{
    RenamePlan plan;

    if (!url.isLocalFile()) {
        plan.error = tr("Unsupported location.");
        return plan;
    }

    const QFileInfo file_info(url.toLocalFile());
    const QString old_name = file_info.baseName();
    const QString trimmed_name = new_base_name.trimmed();

    if (trimmed_name.isEmpty()) {
        plan.error = tr("Name cannot be empty.");
        return plan;
    }
    if (trimmed_name == old_name)
        return plan;

    if (trimmed_name.contains(QRegularExpression("[/\\\\:*?\"<>|]"))) {
        plan.error = tr("Name contains invalid characters.");
        return plan;
    }

    const QString extension = file_info.suffix();
    const QString new_display_name =
        extension.isEmpty() ? trimmed_name
                            : trimmed_name + QLatin1Char('.') + extension;

    const QString new_path =
        QDir(file_info.absolutePath()).filePath(new_display_name);
    if (QFile::exists(new_path)) {
        plan.error = tr("A file with this name already exists.");
        return plan;
    }

    plan.new_display_name = new_display_name;
    plan.changed = true;
    return plan;
}

QString DesktopGifStorage::removeFile(const QUrl &url)
{
    if (!url.isLocalFile())
        return tr("Unsupported location.");

    QFile file(url.toLocalFile());
    if (!file.remove())
        return tr("Failed to delete file: %1").arg(QFileInfo(url.toLocalFile()).fileName());
    return {};
}

QString DesktopGifStorage::renameFile(const QUrl &url,
                                      const QString &new_display_name)
{
    if (!url.isLocalFile())
        return tr("Unsupported location.");

    const QFileInfo file_info(url.toLocalFile());
    const QString new_path =
        QDir(file_info.absolutePath()).filePath(new_display_name);

    if (!QFile::rename(url.toLocalFile(), new_path))
        return tr("Failed to rename file.");
    return {};
}
