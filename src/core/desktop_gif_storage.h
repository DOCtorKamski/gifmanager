#ifndef DESKTOP_GIF_STORAGE_H
#define DESKTOP_GIF_STORAGE_H

#include "gif_storage.h"

class DesktopGifStorage : public GifStorage
{
public:
    bool exists(const QUrl &container) const override;
    QList<GifItem> listGifs(const QUrl &container,
                            const StopPredicate &shouldStop = {}) override;
    QImage readThumbnail(const QUrl &url, const QSize &size) override;
};

#endif // DESKTOP_GIF_STORAGE_H
