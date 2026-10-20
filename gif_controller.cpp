#include "gif_controller.h"

#include "gif_loader_worker.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QPixmap>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QThread>
#include <QVariant>

GifController::GifController(ThumbnailProvider *provider, QObject *parent)
    : QObject(parent)
    , gif_model(new GifModel(provider, this))
    , proxy_model(new QSortFilterProxyModel(this))
{
    proxy_model->setSourceModel(gif_model);
    proxy_model->setFilterRole(GifModel::FileNameRole);
    proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_model->setDynamicSortFilter(true);
}

GifController::~GifController()
{
    shutdown();
}

QAbstractItemModel *GifController::proxyModel() const
{
    return proxy_model;
}

bool GifController::isLoading() const
{
    return is_loading;
}

int GifController::progressCurrent() const
{
    return progress_current;
}

int GifController::progressTotal() const
{
    return progress_total;
}

QString GifController::currentFolder() const
{
    return current_folder;
}

QUrl GifController::currentFolderUrl() const
{
    return current_folder.isEmpty() ? QUrl() : QUrl::fromLocalFile(current_folder);
}

QString GifController::removingPath() const
{
    return removing_path;
}

void GifController::loadFolder(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists()) {
        emit operationError(tr("Folder not found: %1").arg(path));
        return;
    }

    stopLoading();

    current_folder = path;
    emit currentFolderChanged();

    QDir directory(path);
    directory.setNameFilters({"*.gif", "*.GIF"});
    const QFileInfoList files = directory.entryInfoList(QDir::Files, QDir::Name);

    all_gif_files.clear();
    all_gif_files.reserve(files.size());
    for (const QFileInfo &file_info : files)
        all_gif_files.append(file_info.absoluteFilePath());

    filter_query.clear();
    proxy_model->setFilterFixedString(QString());
    emit filterCleared();

    startLoading();
}

void GifController::loadFolderUrl(const QUrl &url)
{
    loadFolder(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

void GifController::loadDefaultFolder()
{
    const QString dir = SettingsManager::instance().getDefaultFolder();
    if (dir.isEmpty()) {
        emit operationError(tr("No default folder is set. Please configure it in Settings."));
        return;
    }
    if (!QDir(dir).exists()) {
        emit operationError(tr("The default folder '%1' does not exist.").arg(dir));
        return;
    }

    loadFolder(dir);
}

void GifController::setFilterQuery(const QString &query)
{
    filter_query = query.trimmed();
    proxy_model->setFilterFixedString(filter_query);
}

void GifController::startLoading()
{
    if (is_loading)
        stopLoading();

    gif_model->resetFiles(all_gif_files);

    ++current_load_id;
    const quint64 load_id = current_load_id;

    progress_current = 0;
    progress_total = all_gif_files.size();
    emit progressChanged();

    is_loading = true;
    emit loadingChanged();

    loaded_thumbnail_size = SettingsManager::instance().getThumbnailSize();

    loader_thread = new QThread(this);
    current_worker = new GifLoaderWorker(all_gif_files,
                                         loaded_thumbnail_size,
                                         load_id);
    current_worker->moveToThread(loader_thread);

    connect(loader_thread, &QThread::started,
            current_worker, &GifLoaderWorker::process);
    connect(current_worker, &GifLoaderWorker::gifLoaded,
            this, &GifController::onGifLoaded, Qt::QueuedConnection);
    connect(current_worker, &GifLoaderWorker::progress,
            this, &GifController::onProgress, Qt::QueuedConnection);
    connect(current_worker, &GifLoaderWorker::finished,
            this, &GifController::onFinished, Qt::QueuedConnection);

    loader_thread->start();
}

void GifController::stopLoading()
{
    if (current_worker)
        current_worker->requestStop();

    if (loader_thread) {
        loader_thread->quit();
        if (loader_thread->isRunning() && !loader_thread->wait(5000)) {
            qWarning() << "Loader thread did not stop within timeout";
            loader_thread->terminate();
            loader_thread->wait();
        }
    }

    delete current_worker;
    current_worker = nullptr;

    delete loader_thread;
    loader_thread = nullptr;

    if (is_loading) {
        is_loading = false;
        emit loadingChanged();
    }
}

void GifController::cleanupFinishedLoader()
{
    if (loader_thread) {
        loader_thread->quit();
        loader_thread->wait();
    }

    delete current_worker;
    current_worker = nullptr;

    delete loader_thread;
    loader_thread = nullptr;
}

void GifController::reloadCurrentFolder()
{
    if (current_folder.isEmpty())
        return;

    stopLoading();
    startLoading();
}

void GifController::onGifLoaded(quint64 load_id, const QString &path, const QImage &image)
{
    if (load_id != current_load_id || !is_loading)
        return;

    gif_model->setThumbnail(path, image);
}

void GifController::onProgress(quint64 load_id, int current, int total)
{
    if (load_id != current_load_id || !is_loading)
        return;

    progress_current = current;
    progress_total = total;
    emit progressChanged();
}

void GifController::onFinished(quint64 load_id)
{
    if (load_id != current_load_id || !is_loading)
        return;

    progress_current = progress_total;
    emit progressChanged();

    is_loading = false;
    emit loadingChanged();

    cleanupFinishedLoader();
}

void GifController::copyToClipboard(const QString &path)
{
    QMimeData *mime = new QMimeData;

    mime->setData("application/x-qt-windows-mime;value=\"FileNameW\"",
                  QFileInfo(path).fileName().toUtf8());

    QImageReader reader(path);
    const QImage image = reader.read();
    if (!image.isNull()) {
        mime->setImageData(QVariant::fromValue(
            QPixmap::fromImage(image)));
    }

    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(path));
    mime->setUrls(urls);

    QGuiApplication::clipboard()->setMimeData(mime);
}

QString GifController::deleteFile(const QString &path)
{
    const QFileInfo file_info(path);
    if (!file_info.exists())
        return tr("File does not exist.");

    setRemovingPath(path);
    emit fileAboutToBeRemoved(path);

    stopLoading();
    gif_model->removeFile(path);

    QFile file(path);
    if (!file.remove()) {
        setRemovingPath(QString());
        reloadCurrentFolder();
        return tr("Failed to delete file: %1").arg(file_info.fileName());
    }

    all_gif_files.removeAll(path);
    emit fileRemoved(path);
    setRemovingPath(QString());
    reloadCurrentFolder();
    return {};
}

QString GifController::renameFile(const QString &path, const QString &new_base_name)
{
    const QFileInfo file_info(path);
    if (!file_info.exists())
        return tr("File does not exist.");

    const QString old_name = file_info.baseName();
    const QString trimmed_name = new_base_name.trimmed();

    if (trimmed_name.isEmpty())
        return tr("Name cannot be empty.");
    if (trimmed_name == old_name)
        return {};
    if (trimmed_name.contains(QRegularExpression("[/\\\\:*?\"<>|]")))
        return tr("Name contains invalid characters.");

    const QString extension = file_info.suffix();
    const QString new_file_path = QDir(file_info.absolutePath()).filePath(
        extension.isEmpty() ? trimmed_name : trimmed_name + QLatin1Char('.') + extension);

    if (QFile::exists(new_file_path))
        return tr("A file with this name already exists.");

    setRemovingPath(path);
    emit fileAboutToBeRemoved(path);

    stopLoading();
    gif_model->removeFile(path);

    if (!QFile::rename(path, new_file_path)) {
        setRemovingPath(QString());
        reloadCurrentFolder();
        return tr("Failed to rename file.");
    }

    all_gif_files.removeAll(path);
    all_gif_files.append(new_file_path);

    emit fileRenamed(path, new_file_path);
    setRemovingPath(QString());
    reloadCurrentFolder();
    return {};
}

void GifController::applySettings()
{
    const QSize thumbnail_size = SettingsManager::instance().getThumbnailSize();
    if (thumbnail_size != loaded_thumbnail_size)
        reloadCurrentFolder();
}

void GifController::shutdown()
{
    stopLoading();
}

void GifController::setRemovingPath(const QString &path)
{
    if (removing_path == path)
        return;

    removing_path = path;
    emit removingPathChanged();
}

QString GifController::displayName(const QString &path) const
{
    return QFileInfo(path).fileName();
}

QString GifController::baseNameForPath(const QString &path) const
{
    return QFileInfo(path).baseName();
}

QUrl GifController::localFileUrl(const QString &path) const
{
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QString GifController::localPathFromUrl(const QUrl &url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}
