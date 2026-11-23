#ifndef GIF_CONTROLLER_H
#define GIF_CONTROLLER_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>

class GifLibrary;
class GifStorage;
class ThumbnailProvider;

class GifController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *proxyModel READ proxyModel CONSTANT)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int progressCurrent READ progressCurrent NOTIFY progressChanged)
    Q_PROPERTY(int progressTotal READ progressTotal NOTIFY progressChanged)
    Q_PROPERTY(QUrl currentFolderUrl READ currentFolderUrl NOTIFY currentFolderChanged)
    Q_PROPERTY(bool hasFolder READ hasFolder NOTIFY currentFolderChanged)
    Q_PROPERTY(QUrl mutatingUrl READ mutatingUrl NOTIFY mutatingUrlChanged)

public:
    explicit GifController(ThumbnailProvider *provider, QObject *parent = nullptr);
    GifController(ThumbnailProvider *provider, std::shared_ptr<GifStorage> storage,
                  QObject *parent = nullptr);
    ~GifController() override;

    QAbstractItemModel *proxyModel() const;
    bool isLoading() const;
    int progressCurrent() const;
    int progressTotal() const;
    QUrl currentFolderUrl() const;
    bool hasFolder() const;
    QUrl mutatingUrl() const;

    Q_INVOKABLE QString loadFolder(const QUrl &url);
    Q_INVOKABLE QString loadFolderUrl(const QUrl &url);
    Q_INVOKABLE QString loadDefaultFolder();
    Q_INVOKABLE void setFilterQuery(const QString &query);
    Q_INVOKABLE QString deleteFile(const QUrl &url);
    Q_INVOKABLE QString renameFile(const QUrl &url, const QString &new_base_name);
    Q_INVOKABLE QUrl playbackUrl(const QUrl &fileUrl) const;
    Q_INVOKABLE void applySettings();
    Q_INVOKABLE void shutdown();

signals:
    void loadingChanged();
    void progressChanged();
    void currentFolderChanged();
    void filterCleared();
    // Forwarded from GifLibrary when the current folder cannot be listed.
    void loadFailed(const QString &message);
    void fileAboutToChange(const QUrl &url);
    void mutatingUrlChanged();

private:
    void beginFileMutation(const QUrl &url);
    void finishFileMutation();
    void setMutatingUrl(const QUrl &url);

    std::shared_ptr<GifStorage> storage;
    GifLibrary *library = nullptr;
    QUrl mutating_url;
};

#endif // GIF_CONTROLLER_H
