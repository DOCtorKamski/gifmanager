#include "gif_controller.h"

#include "gif_loader_task.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QClipboard>
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

QString GifController::mutatingPath() const
{
    return mutating_path;
}

QString GifController::loadFolder(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists())
        return tr("Folder not found: %1").arg(path);

    stopLoading();

    current_folder = path;
    emit currentFolderChanged();

    proxy_model->setFilterFixedString(QString());
    emit filterCleared();

    startLoading();
    return {};
}

QString GifController::loadFolderUrl(const QUrl &url)
{
    return loadFolder(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

QString GifController::loadDefaultFolder()
{
    const QString dir = SettingsManager::instance().getDefaultFolder();
    if (dir.isEmpty())
        return tr("No default folder is set. Please configure it in Settings.");
    if (!QDir(dir).exists())
        return tr("The default folder '%1' does not exist.").arg(dir);

    return loadFolder(dir);
}

void GifController::setFilterQuery(const QString &query)
{
    proxy_model->setFilterFixedString(query.trimmed());
}

void GifController::startLoading()
{
    if (current_task)
        stopLoading();

    gif_model->resetFiles(QStringList());

    progress_current = 0;
    progress_total = 0;
    emit progressChanged();

    is_loading = true;
    emit loadingChanged();

    loaded_thumbnail_size = SettingsManager::instance().getThumbnailSize();

    const GifLoadRequest request =
        GifLoadRequest::fromFolder(current_folder, loaded_thumbnail_size);

    current_task = new GifLoaderTask(request, this);
    connect(current_task, &GifLoaderTask::filesDiscovered,
            this, &GifController::onFilesDiscovered);
    connect(current_task, &GifLoaderTask::gifLoaded,
            this, &GifController::onGifLoaded);
    connect(current_task, &GifLoaderTask::progress,
            this, &GifController::onProgress);
    connect(current_task, &GifLoaderTask::finished,
            this, &GifController::onFinished);

    current_task->start();
}

void GifController::stopLoading()
{
    if (current_task) {
        current_task->cancel();
        delete current_task;
        current_task = nullptr;
    }

    if (is_loading) {
        is_loading = false;
        emit loadingChanged();
    }
}

void GifController::reloadCurrentFolder()
{
    if (current_folder.isEmpty())
        return;

    // Always rescan so files added or removed outside the application are
    // picked up as well.
    stopLoading();
    startLoading();
}

void GifController::onFilesDiscovered(const QStringList &file_paths)
{
    if (!is_loading)
        return;

    gif_model->resetFiles(file_paths);

    progress_current = 0;
    progress_total = file_paths.size();
    emit progressChanged();
}

void GifController::onGifLoaded(const QString &path, const QImage &image)
{
    if (!is_loading)
        return;

    gif_model->setThumbnail(path, image);
}

void GifController::onProgress(int current, int total)
{
    if (!is_loading)
        return;

    progress_current = current;
    progress_total = total;
    emit progressChanged();
}

void GifController::onFinished()
{
    if (!is_loading)
        return;

    // Clear the task before emitting anything: a handler may start a new load.
    GifLoaderTask *finished_task = current_task;
    current_task = nullptr;

    progress_current = progress_total;
    emit progressChanged();

    is_loading = false;
    emit loadingChanged();

    if (finished_task)
        finished_task->deleteLater();
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

    beginFileMutation(path);

    QFile file(path);
    if (!file.remove()) {
        finishFileMutation();
        return tr("Failed to delete file: %1").arg(file_info.fileName());
    }

    finishFileMutation();
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

    beginFileMutation(path);

    if (!QFile::rename(path, new_file_path)) {
        finishFileMutation();
        return tr("Failed to rename file.");
    }

    finishFileMutation();
    return {};
}

void GifController::beginFileMutation(const QString &path)
{
    setMutatingPath(path);
    emit fileAboutToChange(path);

    stopLoading();
    gif_model->removeFile(path);
}

void GifController::finishFileMutation()
{
    setMutatingPath(QString());
    reloadCurrentFolder();
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

void GifController::setMutatingPath(const QString &path)
{
    if (mutating_path == path)
        return;

    mutating_path = path;
    emit mutatingPathChanged();
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
