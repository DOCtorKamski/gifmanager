#include "playback_cache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

PlaybackCache::PlaybackCache()
{
    clear();
}

QString PlaybackCache::directory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QLatin1String("/playback");
}

QString PlaybackCache::filePath(const QUrl &source)
{
    const QString hex = QString::fromLatin1(
        QCryptographicHash::hash(source.toString().toUtf8(),
                                 QCryptographicHash::Sha1).toHex());
    return directory() + QLatin1Char('/') + hex + QLatin1String(".gif");
}

QUrl PlaybackCache::cachedUrl(const QUrl &source) const
{
    const QString path = filePath(source);
    if (!QFileInfo::exists(path))
        return {};
    return QUrl::fromLocalFile(path);
}

QUrl PlaybackCache::store(const QUrl &source, const QByteArray &data)
{
    const QUrl cached = cachedUrl(source);
    if (!cached.isEmpty())
        return cached;

    if (data.isEmpty() || !QDir().mkpath(directory()))
        return {};

    const QString path = filePath(source);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()
        || !file.commit()) {
        return {};
    }
    return QUrl::fromLocalFile(path);
}

void PlaybackCache::clear() const
{
    QDir(directory()).removeRecursively();
}
