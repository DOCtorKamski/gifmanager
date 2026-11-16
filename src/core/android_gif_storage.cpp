#include "android_gif_storage.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJniEnvironment>
#include <QJniObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>

#include <algorithm>

namespace {

constexpr const char *SAF_STORAGE_CLASS = "org/qtproject/gifmanager/SafStorage";

QJniObject androidContext()
{
    return QNativeInterface::QAndroidApplication::context();
}

QString jniString(const QJniObject &value)
{
    return value.isValid() ? value.toString() : QString();
}

QString displayNameFor(const QUrl &url)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    return jniString(QJniObject::callStaticObjectMethod(
        SAF_STORAGE_CLASS, "displayName",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        context.object(), uri.object()));
}

bool documentExists(const QUrl &url)
{
    return !displayNameFor(url).isEmpty();
}

bool childExists(const QUrl &url, const QString &display_name)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject name = QJniObject::fromString(display_name);
    return QJniObject::callStaticMethod<jboolean>(
        SAF_STORAGE_CLASS, "childExists",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        context.object(), uri.object(), name.object());
}

QStringList listContent(const QUrl &container)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(container.toString());
    const QJniObject result = QJniObject::callStaticObjectMethod(
        SAF_STORAGE_CLASS, "listGifs",
        "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
        context.object(), uri.object());

    if (!result.isValid())
        return {};

    QJniEnvironment env;
    auto array = static_cast<jobjectArray>(result.object());
    const jsize count = env->GetArrayLength(array);
    QStringList values;
    values.reserve(count);

    for (jsize i = 0; i < count; ++i) {
        // fromLocalRef() adopts the local ref, so the element does not need a
        // manual DeleteLocalRef() and QJniObject::toString() handles Unicode
        // file names correctly.
        auto element = env->GetObjectArrayElement(array, i);
        if (!element) {
            values.append(QString());
            continue;
        }
        values.append(QJniObject::fromLocalRef(element).toString());
    }

    return values;
}

QByteArray readContent(const QUrl &url)
{
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject result = QJniObject::callStaticObjectMethod(
        SAF_STORAGE_CLASS, "readFile",
        "(Landroid/content/Context;Ljava/lang/String;)[B",
        context.object(), uri.object());

    if (!result.isValid())
        return {};

    QJniEnvironment env;
    auto array = static_cast<jbyteArray>(result.object());
    const jsize length = env->GetArrayLength(array);
    if (length <= 0)
        return {};

    QByteArray data;
    data.resize(length);
    env->GetByteArrayRegion(array, 0, length,
                            reinterpret_cast<jbyte *>(data.data()));
    return data;
}

// Playback cache: QML AnimatedImage cannot open content:// URIs, so each GIF
// is copied to a per-session cache file and served as a file:// URL. Copies
// are pre-warmed by readThumbnail() on the loader thread; playbackUrl() only
// reads SAF data when the copy is still missing.
QString playbackCacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QLatin1String("/playback");
}

QString playbackFilePath(const QUrl &url)
{
    const QString hex = QString::fromLatin1(
        QCryptographicHash::hash(url.toString().toUtf8(),
                                 QCryptographicHash::Sha1).toHex());
    return playbackCacheDir() + QLatin1Char('/') + hex
           + QLatin1String(".gif");
}

QUrl writePlaybackCopy(const QUrl &url, const QByteArray &data)
{
    const QString path = playbackFilePath(url);
    if (QFileInfo::exists(path))
        return QUrl::fromLocalFile(path);

    QDir().mkpath(playbackCacheDir());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()
        || !file.commit()) {
        return url;
    }
    return QUrl::fromLocalFile(path);
}

} // namespace

AndroidGifStorage::AndroidGifStorage()
{
    // Playback cache copies are only valid for one session: clear them so
    // deleted/renamed documents cannot keep serving stale content.
    QDir(playbackCacheDir()).removeRecursively();
}

bool AndroidGifStorage::exists(const QUrl &container) const
{
    if (container.isLocalFile())
        return desktop.exists(container);
    if (container.scheme() != QLatin1String("content"))
        return false;

    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(container.toString());

    // The native folder dialog grants persistable access; taking it again is
    // harmless and also covers URIs restored from settings after a restart.
    QJniObject::callStaticMethod<jboolean>(
        SAF_STORAGE_CLASS, "takePersistablePermission",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object(), uri.object());

    return QJniObject::callStaticMethod<jboolean>(
        SAF_STORAGE_CLASS, "isReadable",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object(), uri.object());
}

QList<GifItem> AndroidGifStorage::listGifs(const QUrl &container,
                                           const StopPredicate &shouldStop)
{
    if (container.isLocalFile())
        return desktop.listGifs(container, shouldStop);
    if (container.scheme() != QLatin1String("content"))
        return {};

    const QStringList values = listContent(container);
    QList<GifItem> items;
    items.reserve(values.size() / 2);

    for (qsizetype i = 0; i + 1 < values.size(); i += 2) {
        if (shouldStop && shouldStop())
            break;

        const QUrl url(values.at(i));
        if (url.isEmpty())
            continue;
        items.append({url, values.at(i + 1)});
    }

    std::sort(items.begin(), items.end(),
              [](const GifItem &lhs, const GifItem &rhs) {
                  return lhs.fileName < rhs.fileName;
              });

    return items;
}

QImage AndroidGifStorage::readThumbnail(const QUrl &url, const QSize &size)
{
    if (url.isLocalFile())
        return desktop.readThumbnail(url, size);
    if (url.scheme() != QLatin1String("content"))
        return {};

    const QByteArray data = readContent(url);
    if (data.isEmpty())
        return {};

    // Pre-warm the playback cache on the loader thread so QML never has to
    // read the whole document synchronously before animating.
    writePlaybackCopy(url, data);

    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly))
        return {};

    QImageReader reader(&buffer);
    reader.setScaledSize(size);
    const QImage image = reader.read();
    if (image.isNull())
        return {};

    return image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QUrl AndroidGifStorage::playbackUrl(const QUrl &url)
{
    if (url.isLocalFile())
        return desktop.playbackUrl(url);
    if (url.scheme() != QLatin1String("content"))
        return url;

    const QString path = playbackFilePath(url);
    if (QFileInfo::exists(path))
        return QUrl::fromLocalFile(path);

    // The copy is normally pre-warmed by readThumbnail(); this synchronous
    // fallback keeps playback working when a thumbnail was skipped (e.g.
    // cancelled load).
    const QByteArray data = readContent(url);
    if (data.isEmpty())
        return url;
    return writePlaybackCopy(url, data);
}

QString AndroidGifStorage::validateFile(const QUrl &url) const
{
    if (url.isLocalFile())
        return desktop.validateFile(url);
    if (url.scheme() != QLatin1String("content"))
        return tr("Unsupported location.");
    if (!documentExists(url))
        return tr("File does not exist.");
    return {};
}

GifStorage::RenamePlan AndroidGifStorage::planRename(
    const QUrl &url, const QString &new_base_name) const
{
    if (url.isLocalFile())
        return desktop.planRename(url, new_base_name);
    if (url.scheme() != QLatin1String("content")) {
        RenamePlan plan;
        plan.error = tr("Unsupported location.");
        return plan;
    }

    RenamePlan plan;
    const QString old_display_name = displayNameFor(url);
    if (old_display_name.isEmpty()) {
        plan.error = tr("File does not exist.");
        return plan;
    }

    const QFileInfo file_info(old_display_name);
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

    if (childExists(url, new_display_name)) {
        plan.error = tr("A file with this name already exists.");
        return plan;
    }

    plan.new_display_name = new_display_name;
    plan.changed = true;
    return plan;
}

QString AndroidGifStorage::removeFile(const QUrl &url)
{
    if (url.isLocalFile())
        return desktop.removeFile(url);
    if (url.scheme() != QLatin1String("content"))
        return tr("Unsupported location.");

    const QString display_name = displayNameFor(url);
    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const bool removed = QJniObject::callStaticMethod<jboolean>(
        SAF_STORAGE_CLASS, "deleteDocument",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object(), uri.object());

    if (!removed) {
        return tr("Failed to delete file: %1")
            .arg(display_name.isEmpty() ? url.toString() : display_name);
    }
    return {};
}

QString AndroidGifStorage::renameFile(const QUrl &url,
                                      const QString &new_display_name)
{
    if (url.isLocalFile())
        return desktop.renameFile(url, new_display_name);
    if (url.scheme() != QLatin1String("content"))
        return tr("Unsupported location.");

    const QJniObject context = androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject display_name = QJniObject::fromString(new_display_name);
    const bool renamed = QJniObject::callStaticMethod<jboolean>(
        SAF_STORAGE_CLASS, "renameDocument",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        context.object(), uri.object(), display_name.object());

    if (!renamed)
        return tr("Failed to rename file.");
    return {};
}
