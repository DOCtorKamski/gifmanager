#ifndef GIF_CONTROLLER_H
#define GIF_CONTROLLER_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QUrl>

class GifLibrary;
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
    Q_PROPERTY(QString mutatingPath READ mutatingPath NOTIFY mutatingPathChanged)

public:
    explicit GifController(ThumbnailProvider *provider, QObject *parent = nullptr);
    ~GifController() override;

    QAbstractItemModel *proxyModel() const;
    bool isLoading() const;
    int progressCurrent() const;
    int progressTotal() const;
    QString currentFolder() const;
    QUrl currentFolderUrl() const;
    QString mutatingPath() const;

    Q_INVOKABLE QString loadFolder(const QString &path);
    Q_INVOKABLE QString loadFolderUrl(const QUrl &url);
    Q_INVOKABLE QString loadDefaultFolder();
    Q_INVOKABLE void setFilterQuery(const QString &query);
    Q_INVOKABLE QString deleteFile(const QString &path);
    Q_INVOKABLE QString renameFile(const QString &path, const QString &new_base_name);
    Q_INVOKABLE void applySettings();
    Q_INVOKABLE void shutdown();

signals:
    void loadingChanged();
    void progressChanged();
    void currentFolderChanged();
    void filterCleared();
    void fileAboutToChange(const QString &path);
    void mutatingPathChanged();

private:
    void beginFileMutation(const QString &path);
    void finishFileMutation();
    void setMutatingPath(const QString &path);

    GifLibrary *library = nullptr;
    QString mutating_path;
};

#endif // GIF_CONTROLLER_H
