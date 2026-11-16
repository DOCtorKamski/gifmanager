#include "gif_library.h"

#include "gif_loader_task.h"
#include "gif_model.h"
#include "gif_storage_factory.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QSortFilterProxyModel>

#include <utility>

namespace {

QString displayLocation(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

} // namespace

GifLibrary::GifLibrary(ThumbnailProvider *provider, QObject *parent)
    : GifLibrary(provider, createPlatformGifStorage(), parent)
{
}

GifLibrary::GifLibrary(ThumbnailProvider *provider,
                       std::shared_ptr<GifStorage> storage_,
                       QObject *parent)
    : QObject(parent)
    , gif_model(new GifModel(provider, this))
    , proxy_model(new QSortFilterProxyModel(this))
    , storage(std::move(storage_))
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
    return !current_task.isNull();
}

int GifLibrary::progressCurrent() const
{
    return progress_current;
}

int GifLibrary::progressTotal() const
{
    return progress_total;
}

QUrl GifLibrary::currentFolderUrl() const
{
    return current_container;
}

QString GifLibrary::loadFolder(const QUrl &container)
{
    if (container.isEmpty() || !storage->exists(container))
        return tr("Folder not found: %1").arg(displayLocation(container));

    stopLoading();

    current_container = container;
    emit currentFolderChanged();

    proxy_model->setFilterFixedString(QString());
    emit filterCleared();

    startLoading();
    return {};
}

QString GifLibrary::loadFolderUrl(const QUrl &url)
{
    return loadFolder(url);
}

QString GifLibrary::loadDefaultFolder()
{
    const QUrl dir = SettingsManager::instance().getDefaultFolderUrl();
    if (dir.isEmpty())
        return tr("No default folder is set. Please configure it in Settings.");
    if (!storage->exists(dir))
        return tr("The default folder '%1' does not exist.").arg(displayLocation(dir));

    return loadFolder(dir);
}

void GifLibrary::setFilterQuery(const QString &query)
{
    proxy_model->setFilterFixedString(query.trimmed());
}

void GifLibrary::startLoading()
{
    // Defensive: a reentrant handler may have started a load while we were
    // emitting currentFolderChanged/filterCleared. Normal callers cancel first.
    if (current_task) {
        stopLoading();
        if (current_task)
            return;
    }

    gif_model->resetFiles(QList<GifItem>());

    if (current_task)
        return;

    loaded_thumbnail_size = SettingsManager::instance().getThumbnailSize();

    const GifLoadRequest request =
        GifLoadRequest::fromContainer(current_container, loaded_thumbnail_size,
                                      storage);

    GifLoaderTask *task = new GifLoaderTask(request, this);
    current_task = task;
    connect(task, &GifLoaderTask::filesDiscovered,
            this, &GifLibrary::onFilesDiscovered);
    connect(task, &GifLoaderTask::gifLoaded,
            this, &GifLibrary::onGifLoaded);
    connect(task, &GifLoaderTask::progress,
            this, &GifLibrary::onProgress);
    connect(task, &GifLoaderTask::finished,
            this, &GifLibrary::onFinished);
    connect(task, &GifLoaderTask::finished,
            task, &QObject::deleteLater);

    task->start();

    setProgress(0, 0);

    // A progressChanged handler may have cancelled or replaced this load.
    if (current_task.data() == task)
        emit loadingChanged();
}

void GifLibrary::stopLoading()
{
    if (!current_task)
        return;

    GifLoaderTask *task = current_task;
    current_task = nullptr;

    // cancel() stops the worker thread synchronously; deleteLater() keeps the
    // sender alive if stopLoading() was called from one of its signal handlers.
    task->cancel();
    task->deleteLater();

    emit loadingChanged();
}

void GifLibrary::reloadCurrentFolder()
{
    if (current_container.isEmpty())
        return;

    // Always rescan so files added or removed outside the application are
    // picked up as well.
    stopLoading();
    startLoading();
}

void GifLibrary::onFilesDiscovered(const QList<GifItem> &items)
{
    if (!current_task)
        return;

    gif_model->resetFiles(items);

    setProgress(0, items.size());
}

void GifLibrary::onGifLoaded(const QUrl &url, const QImage &image)
{
    if (!current_task)
        return;

    gif_model->setThumbnail(url, image);
}

void GifLibrary::onProgress(int current, int total)
{
    if (!current_task)
        return;

    setProgress(current, total);
}

void GifLibrary::onFinished()
{
    if (!current_task)
        return;

    // Clear the task before emitting anything: a handler may start a new load.
    current_task = nullptr;

    setProgress(progress_total, progress_total);

    // Do not emit a stale "not loading" if progressChanged started a new load.
    if (!current_task)
        emit loadingChanged();
}

void GifLibrary::setProgress(int current, int total)
{
    progress_current = current;
    progress_total = total;
    emit progressChanged();
}

void GifLibrary::applySettings()
{
    const QSize thumbnail_size = SettingsManager::instance().getThumbnailSize();
    if (thumbnail_size != loaded_thumbnail_size)
        reloadCurrentFolder();
}

void GifLibrary::removeModelEntry(const QUrl &url)
{
    gif_model->removeFile(url);
}

void GifLibrary::shutdown()
{
    stopLoading();
}
