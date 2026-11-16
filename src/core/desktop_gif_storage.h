#ifndef DESKTOP_GIF_STORAGE_H
#define DESKTOP_GIF_STORAGE_H

#include "gif_storage.h"

#include <QCoreApplication>

class DesktopGifStorage : public GifStorage
{
    Q_DECLARE_TR_FUNCTIONS(DesktopGifStorage)
public:
    bool exists(const QUrl &container) const override;
    QList<GifItem> listGifs(const QUrl &container,
                            const StopPredicate &shouldStop = {}) override;
    QImage readThumbnail(const QUrl &url, const QSize &size) override;

    QString validateFile(const QUrl &url) const override;
    RenamePlan planRename(const QUrl &url,
                          const QString &new_base_name) const override;
    QString removeFile(const QUrl &url) override;
    QString renameFile(const QUrl &url,
                       const QString &new_display_name) override;
};

#endif // DESKTOP_GIF_STORAGE_H
