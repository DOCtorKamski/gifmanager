#include "android_gif_storage.h"
#include "jni_util.h"

#include <QDebug>
#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QJniEnvironment>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

namespace {

QString displayNameFor(const QUrl &url)
{
    return jni::string(
        jni::callStaticString(jni::SAF_STORAGE_CLASS, "displayName", url));
}

bool documentExists(const QUrl &url)
{
    return !displayNameFor(url).isEmpty();
}

bool childExists(const QUrl &url, const QString &display_name)
{
    return jni::callStaticBool(jni::SAF_STORAGE_CLASS, "childExists", url,
                               display_name);
}

bool listContent(const QUrl &container, QStringList *values)
{
    const QJniObject context = jni::androidContext();
    const QJniObject uri = QJniObject::fromString(container.toString());
    const QJniObject result = QJniObject::callStaticObjectMethod(
        jni::SAF_STORAGE_CLASS, "listGifs",
        "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
        context.object(), uri.object());

    if (!result.isValid()) {
        // The Java helper returns null on failure and an empty array for an
        // empty folder; the caller turns this into a user-visible error.
        qWarning() << "SafStorage.listGifs failed for" << container.toString();
        return false;
    }

    QJniEnvironment env;
    auto array = static_cast<jobjectArray>(result.object());
    const jsize count = env->GetArrayLength(array);
    values->clear();
    values->reserve(count);

    for (jsize i = 0; i < count; ++i) {
        // fromLocalRef() adopts the local ref, so the element does not need a
        // manual DeleteLocalRef() and QJniObject::toString() handles Unicode
        // file names correctly.
        auto element = env->GetObjectArrayElement(array, i);
        if (!element) {
            values->append(QString());
            continue;
        }
        values->append(QJniObject::fromLocalRef(element).toString());
    }

    return true;
}

QByteArray readContent(const QUrl &url)
{
    const QJniObject context = jni::androidContext();
    const QJniObject uri = QJniObject::fromString(url.toString());
    const QJniObject result = QJniObject::callStaticObjectMethod(
        jni::SAF_STORAGE_CLASS, "readFile",
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

} // namespace

AndroidGifStorage::AndroidGifStorage() = default;

bool AndroidGifStorage::exists(const QUrl &container) const
{
    if (container.isLocalFile())
        return desktop.exists(container);
    if (container.scheme() != QLatin1String("content"))
        return false;

    // The native folder dialog grants persistable access; taking it again is
    // harmless and also covers URIs restored from settings after a restart.
    jni::callStaticBool(jni::SAF_STORAGE_CLASS, "takePersistablePermission",
                        container);

    return jni::callStaticBool(jni::SAF_STORAGE_CLASS, "isReadable", container);
}

GifStorage::Listing AndroidGifStorage::listGifs(const QUrl &container,
                                                const StopPredicate &shouldStop)
{
    if (container.isLocalFile())
        return desktop.listGifs(container, shouldStop);

    Listing listing;
    if (container.scheme() != QLatin1String("content")) {
        listing.error = tr("Unsupported location.");
        return listing;
    }

    QStringList values;
    if (!listContent(container, &values)) {
        listing.error = tr("Folder cannot be read: %1").arg(container.toString());
        return listing;
    }

    listing.items.reserve(values.size() / 2);

    for (qsizetype i = 0; i + 1 < values.size(); i += 2) {
        if (shouldStop && shouldStop())
            break;

        const QUrl url(values.at(i));
        if (url.isEmpty())
            continue;
        listing.items.append({url, values.at(i + 1)});
    }

    std::sort(listing.items.begin(), listing.items.end(),
              [](const GifItem &lhs, const GifItem &rhs) {
                  return lhs.fileName < rhs.fileName;
              });

    return listing;
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
    playback_cache.store(url, data);

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

    const QUrl cached = playback_cache.cachedUrl(url);
    if (!cached.isEmpty())
        return cached;

    // The copy is normally pre-warmed by readThumbnail(); this synchronous
    // fallback keeps playback working when a thumbnail was skipped (e.g.
    // cancelled load).
    const QByteArray data = readContent(url);
    if (data.isEmpty())
        return url;

    const QUrl stored = playback_cache.store(url, data);
    return stored.isEmpty() ? url : stored;
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
    // completeBaseName() keeps inner dots: the rename plan must treat
    // "cat.funny.gif" as "cat.funny", matching the dialog prefill.
    const QString old_name = file_info.completeBaseName();
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
    const bool removed = jni::callStaticBool(
        jni::SAF_STORAGE_CLASS, "deleteDocument", url);

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

    const bool renamed = jni::callStaticBool(
        jni::SAF_STORAGE_CLASS, "renameDocument", url, new_display_name);

    if (!renamed)
        return tr("Failed to rename file.");
    return {};
}
