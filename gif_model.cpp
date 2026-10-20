#include "gif_model.h"
#include "thumbnail_provider.h"

#include <QByteArray>
#include <QFileInfo>

GifModel::GifModel(ThumbnailProvider *provider, QObject *parent)
    : QAbstractListModel(parent)
    , thumbnail_provider(provider)
{
}

int GifModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return items.size();
}

QVariant GifModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items.size())
        return {};

    const Item &item = items.at(index.row());

    switch (role) {
    case FilePathRole:
        return item.path;
    case FileNameRole:
        return item.fileName;
    case FileUrlRole:
        return QUrl::fromLocalFile(item.path);
    case ThumbnailUrlRole:
        if (!item.thumbnailReady || item.imageId.isEmpty())
            return QUrl();
        return QUrl(QStringLiteral("image://gifthumb/") + item.imageId);
    case ThumbnailReadyRole:
        return item.thumbnailReady;
    default:
        return {};
    }
}

QHash<int, QByteArray> GifModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FilePathRole] = "filePath";
    roles[FileNameRole] = "fileName";
    roles[FileUrlRole] = "fileUrl";
    roles[ThumbnailUrlRole] = "thumbnailUrl";
    roles[ThumbnailReadyRole] = "thumbnailReady";
    return roles;
}

void GifModel::resetFiles(const QStringList &paths)
{
    beginResetModel();

    if (thumbnail_provider)
        thumbnail_provider->clear();

    items.clear();
    path_to_row.clear();
    items.reserve(paths.size());

    for (const QString &path : paths) {
        Item item;
        item.path = path;
        item.fileName = QFileInfo(path).fileName();
        path_to_row.insert(path, items.size());
        items.append(item);
    }

    endResetModel();
}

void GifModel::setThumbnail(const QString &path, const QImage &image)
{
    const auto it = path_to_row.constFind(path);
    if (it == path_to_row.constEnd())
        return;

    const int row = it.value();
    Item &item = items[row];

    const QString old_id = item.imageId;
    item.revision++;
    item.imageId = makeImageId(item);
    item.thumbnailReady = true;

    if (thumbnail_provider)
        thumbnail_provider->setImage(item.imageId, image);
    if (!old_id.isEmpty() && thumbnail_provider)
        thumbnail_provider->removeImage(old_id);

    const QModelIndex model_index = index(row, 0);
    emit dataChanged(model_index, model_index,
                     {ThumbnailUrlRole, ThumbnailReadyRole});
}

bool GifModel::removeFile(const QString &path)
{
    const auto it = path_to_row.constFind(path);
    if (it == path_to_row.constEnd())
        return false;

    const int row = it.value();

    if (thumbnail_provider && !items.at(row).imageId.isEmpty())
        thumbnail_provider->removeImage(items.at(row).imageId);

    beginRemoveRows(QModelIndex(), row, row);
    items.removeAt(row);
    endRemoveRows();

    rebuildPathIndex();
    return true;
}

QString GifModel::makeImageId(const Item &item) const
{
    const QString encoded = QString::fromLatin1(
        item.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding
                                    | QByteArray::OmitTrailingEquals));
    return encoded + QLatin1Char('/') + QString::number(item.revision);
}

void GifModel::rebuildPathIndex()
{
    path_to_row.clear();
    for (int i = 0; i < items.size(); ++i)
        path_to_row.insert(items.at(i).path, i);
}
