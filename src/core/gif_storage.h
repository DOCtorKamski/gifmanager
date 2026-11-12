#ifndef GIF_STORAGE_H
#define GIF_STORAGE_H

#include "gif_item.h"

#include <QImage>
#include <QList>
#include <QSize>
#include <QUrl>

#include <functional>

// Platform storage backend. Desktop uses QDir/QFile/QImageReader; Android will
// provide a SAF-backed implementation. Implementations must stay free of UI
// state so the loader thread can use them safely.
class GifStorage
{
public:
    // Lets the loader cancel a long directory scan between entries.
    using StopPredicate = std::function<bool()>;

    virtual ~GifStorage() = default;

    virtual bool exists(const QUrl &container) const = 0;
    virtual QList<GifItem> listGifs(const QUrl &container,
                                    const StopPredicate &shouldStop = {}) = 0;
    virtual QImage readThumbnail(const QUrl &url, const QSize &size) = 0;
};

#endif // GIF_STORAGE_H
