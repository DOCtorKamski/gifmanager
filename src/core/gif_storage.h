#ifndef GIF_STORAGE_H
#define GIF_STORAGE_H

#include "gif_item.h"

#include <QImage>
#include <QList>
#include <QSize>
#include <QString>
#include <QUrl>

#include <functional>

// Platform storage backend. Desktop uses QDir/QFile/QImageReader; Android uses
// a SAF-backed implementation. Implementations must stay free of UI state so
// the loader thread can use them safely.
class GifStorage
{
public:
    // Lets the loader cancel a long directory scan between entries.
    using StopPredicate = std::function<bool()>;

    struct RenamePlan {
        QString error;
        QString new_display_name;
        bool changed = false;
    };

    // Listing result. An empty error means the scan succeeded; a non-empty one
    // means the container could not be read and items is empty, so the UI can
    // tell "folder is broken" apart from "folder contains no GIFs".
    struct Listing {
        QList<GifItem> items;
        QString error;

        bool failed() const { return !error.isEmpty(); }
    };

    virtual ~GifStorage() = default;

    virtual bool exists(const QUrl &container) const = 0;
    virtual Listing listGifs(const QUrl &container,
                             const StopPredicate &shouldStop = {}) = 0;
    virtual QImage readThumbnail(const QUrl &url, const QSize &size) = 0;

    // Returns a URL the QML AnimatedImage can open for full animation.
    // Desktop returns the input unchanged; Android caches the document bytes
    // in the app cache directory (pre-warmed by readThumbnail()) and returns
    // the cached file:// URL, because AnimatedImage cannot open content://
    // URIs directly.
    virtual QUrl playbackUrl(const QUrl &url) = 0;

    // File mutations are platform-specific (local files on desktop, SAF on
    // Android). The controller validates before it removes the model row and
    // then calls removeFile()/renameFile() to perform the operation.
    virtual QString validateFile(const QUrl &url) const = 0;
    virtual RenamePlan planRename(const QUrl &url,
                                  const QString &new_base_name) const = 0;
    virtual QString removeFile(const QUrl &url) = 0;
    virtual QString renameFile(const QUrl &url,
                               const QString &new_display_name) = 0;
};

#endif // GIF_STORAGE_H
