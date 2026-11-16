#ifndef ANDROID_GIF_STORAGE_H
#define ANDROID_GIF_STORAGE_H

#include "desktop_gif_storage.h"
#include "gif_storage.h"

#include <QCoreApplication>

// SAF-backed storage for Android. The Java helper in
// android/src/org/qtproject/gifmanager/SafStorage.java performs the
// DocumentsContract/ContentResolver calls; this class only marshals JNI values
// and keeps the platform-independent GifStorage contract.
class AndroidGifStorage : public GifStorage
{
    Q_DECLARE_TR_FUNCTIONS(AndroidGifStorage)
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

private:
    DesktopGifStorage desktop;
};

#endif // ANDROID_GIF_STORAGE_H
