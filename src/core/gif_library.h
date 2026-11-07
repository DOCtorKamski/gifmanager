#ifndef GIF_LIBRARY_H
#define GIF_LIBRARY_H

#include "gif_loader_task.h"

#include <QAbstractItemModel>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>

class QSortFilterProxyModel;
class GifModel;
class ThumbnailProvider;

class GifLibrary : public QObject
{
    Q_OBJECT

public:
    explicit GifLibrary(ThumbnailProvider *provider, QObject *parent = nullptr);
    ~GifLibrary() override;

    QAbstractItemModel *proxyModel() const;
    bool isLoading() const;
    int progressCurrent() const;
    int progressTotal() const;
    QString currentFolder() const;
    QUrl currentFolderUrl() const;

    QString loadFolder(const QString &path);
    QString loadFolderUrl(const QUrl &url);
    QString loadDefaultFolder();
    void setFilterQuery(const QString &query);
    void applySettings();
    void reloadCurrentFolder();
    void stopLoading();
    void removeModelEntry(const QString &path);
    void shutdown();

signals:
    void loadingChanged();
    void progressChanged();
    void currentFolderChanged();
    void filterCleared();

private slots:
    void onFilesDiscovered(const QStringList &file_paths);
    void onGifLoaded(const QString &path, const QImage &image);
    void onProgress(int current, int total);
    void onFinished();

private:
    void startLoading();
    void setProgress(int current, int total);

    GifModel *gif_model = nullptr;
    QSortFilterProxyModel *proxy_model = nullptr;

    QString current_folder;

    // At most one task is alive at a time: every new load cancels the previous
    // one, which is why stale worker signals no longer need load IDs. The
    // library observes the task; ownership stays with the QObject parent and
    // finished/cancelled tasks delete themselves via deleteLater().
    QPointer<GifLoaderTask> current_task;

    int progress_current = 0;
    int progress_total = 0;
    QSize loaded_thumbnail_size;
};

#endif // GIF_LIBRARY_H
