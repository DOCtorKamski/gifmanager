#ifndef GIF_CONTROLLER_H
#define GIF_CONTROLLER_H

#include <QAbstractItemModel>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QUrl>

class QSortFilterProxyModel;
class QThread;
class GifLoaderWorker;
class GifModel;
class ThumbnailProvider;

class GifController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *proxyModel READ proxyModel CONSTANT)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int progressCurrent READ progressCurrent NOTIFY progressChanged)
    Q_PROPERTY(int progressTotal READ progressTotal NOTIFY progressChanged)
    Q_PROPERTY(QString currentFolder READ currentFolder NOTIFY currentFolderChanged)
    Q_PROPERTY(QUrl currentFolderUrl READ currentFolderUrl NOTIFY currentFolderChanged)
    Q_PROPERTY(QString removingPath READ removingPath NOTIFY removingPathChanged)

public:
    explicit GifController(ThumbnailProvider *provider, QObject *parent = nullptr);
    ~GifController() override;

    QAbstractItemModel *proxyModel() const;
    bool isLoading() const;
    int progressCurrent() const;
    int progressTotal() const;
    QString currentFolder() const;
    QUrl currentFolderUrl() const;
    QString removingPath() const;

    Q_INVOKABLE void loadFolder(const QString &path);
    Q_INVOKABLE void loadFolderUrl(const QUrl &url);
    Q_INVOKABLE void loadDefaultFolder();
    Q_INVOKABLE void setFilterQuery(const QString &query);
    Q_INVOKABLE void copyToClipboard(const QString &path);
    Q_INVOKABLE QString deleteFile(const QString &path);
    Q_INVOKABLE QString renameFile(const QString &path, const QString &new_base_name);
    Q_INVOKABLE void applySettings();
    Q_INVOKABLE void shutdown();

    Q_INVOKABLE QString displayName(const QString &path) const;
    Q_INVOKABLE QString baseNameForPath(const QString &path) const;
    Q_INVOKABLE QUrl localFileUrl(const QString &path) const;
    Q_INVOKABLE QString localPathFromUrl(const QUrl &url) const;

signals:
    void loadingChanged();
    void progressChanged();
    void currentFolderChanged();
    void filterCleared();
    void operationError(const QString &message);
    void fileAboutToBeRemoved(const QString &path);
    void fileRemoved(const QString &path);
    void fileRenamed(const QString &old_path, const QString &new_path);
    void removingPathChanged();

private slots:
    void onGifLoaded(quint64 load_id, const QString &path, const QImage &image);
    void onProgress(quint64 load_id, int current, int total);
    void onFinished(quint64 load_id);

private:
    void startLoading();
    void stopLoading();
    void cleanupFinishedLoader();
    void reloadCurrentFolder();
    void setRemovingPath(const QString &path);

    GifModel *gif_model = nullptr;
    QSortFilterProxyModel *proxy_model = nullptr;

    QStringList all_gif_files;
    QString current_folder;
    QString filter_query;
    QString removing_path;

    QThread *loader_thread = nullptr;
    GifLoaderWorker *current_worker = nullptr;

    bool is_loading = false;
    quint64 current_load_id = 0;
    int progress_current = 0;
    int progress_total = 0;
    QSize loaded_thumbnail_size;
};

#endif // GIF_CONTROLLER_H
