#include "gif_library.h"

#include "gif_loader_task.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QDir>
#include <QSortFilterProxyModel>

GifLibrary::GifLibrary(ThumbnailProvider *provider, QObject *parent)
    : QObject(parent)
    , gif_model(new GifModel(provider, this))
    , proxy_model(new QSortFilterProxyModel(this))
{
    proxy_model->setSourceModel(gif_model);
    proxy_model->setFilterRole(GifModel::FileNameRole);
    proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_model->setDynamicSortFilter(true);
}

GifLibrary::~GifLibrary()
{
    shutdown();
}

QAbstractItemModel *GifLibrary::proxyModel() const
{
    return proxy_model;
}

bool GifLibrary::isLoading() const
{
    return is_loading;
}

int GifLibrary::progressCurrent() const
{
    return progress_current;
}

int GifLibrary::progressTotal() const
{
    return progress_total;
}

QString GifLibrary::currentFolder() const
{
    return current_folder;
}

QUrl GifLibrary::currentFolderUrl() const
{
    return current_folder.isEmpty() ? QUrl() : QUrl::fromLocalFile(current_folder);
}

QString GifLibrary::loadFolder(const QString &path)
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

QString GifLibrary::loadFolderUrl(const QUrl &url)
{
    return loadFolder(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

QString GifLibrary::loadDefaultFolder()
{
    const QString dir = SettingsManager::instance().getDefaultFolder();
    if (dir.isEmpty())
        return tr("No default folder is set. Please configure it in Settings.");
    if (!QDir(dir).exists())
        return tr("The default folder '%1' does not exist.").arg(dir);

    return loadFolder(dir);
}

void GifLibrary::setFilterQuery(const QString &query)
{
    proxy_model->setFilterFixedString(query.trimmed());
}

void GifLibrary::startLoading()
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
            this, &GifLibrary::onFilesDiscovered);
    connect(current_task, &GifLoaderTask::gifLoaded,
            this, &GifLibrary::onGifLoaded);
    connect(current_task, &GifLoaderTask::progress,
            this, &GifLibrary::onProgress);
    connect(current_task, &GifLoaderTask::finished,
            this, &GifLibrary::onFinished);

    current_task->start();
}

void GifLibrary::stopLoading()
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

void GifLibrary::reloadCurrentFolder()
{
    if (current_folder.isEmpty())
        return;

    // Always rescan so files added or removed outside the application are
    // picked up as well.
    stopLoading();
    startLoading();
}

void GifLibrary::onFilesDiscovered(const QStringList &file_paths)
{
    if (!is_loading)
        return;

    gif_model->resetFiles(file_paths);

    progress_current = 0;
    progress_total = file_paths.size();
    emit progressChanged();
}

void GifLibrary::onGifLoaded(const QString &path, const QImage &image)
{
    if (!is_loading)
        return;

    gif_model->setThumbnail(path, image);
}

void GifLibrary::onProgress(int current, int total)
{
    if (!is_loading)
        return;

    progress_current = current;
    progress_total = total;
    emit progressChanged();
}

void GifLibrary::onFinished()
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

void GifLibrary::applySettings()
{
    const QSize thumbnail_size = SettingsManager::instance().getThumbnailSize();
    if (thumbnail_size != loaded_thumbnail_size)
        reloadCurrentFolder();
}

void GifLibrary::removeModelEntry(const QString &path)
{
    gif_model->removeFile(path);
}

void GifLibrary::shutdown()
{
    stopLoading();
}
