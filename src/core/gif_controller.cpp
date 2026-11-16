#include "gif_controller.h"

#include "gif_library.h"
#include "gif_storage.h"
#include "gif_storage_factory.h"

#include <utility>

GifController::GifController(ThumbnailProvider *provider, QObject *parent)
    : GifController(provider, createPlatformGifStorage(), parent)
{
}

GifController::GifController(ThumbnailProvider *provider,
                             std::shared_ptr<GifStorage> storage_,
                             QObject *parent)
    : QObject(parent)
    , storage(std::move(storage_))
    , library(new GifLibrary(provider, storage, this))
{
    connect(library, &GifLibrary::loadingChanged,
            this, &GifController::loadingChanged);
    connect(library, &GifLibrary::progressChanged,
            this, &GifController::progressChanged);
    connect(library, &GifLibrary::currentFolderChanged,
            this, &GifController::currentFolderChanged);
    connect(library, &GifLibrary::filterCleared,
            this, &GifController::filterCleared);
}

GifController::~GifController()
{
    shutdown();
}

QAbstractItemModel *GifController::proxyModel() const
{
    return library->proxyModel();
}

bool GifController::isLoading() const
{
    return library->isLoading();
}

int GifController::progressCurrent() const
{
    return library->progressCurrent();
}

int GifController::progressTotal() const
{
    return library->progressTotal();
}

QUrl GifController::currentFolderUrl() const
{
    return library->currentFolderUrl();
}

bool GifController::hasFolder() const
{
    return !library->currentFolderUrl().isEmpty();
}

QUrl GifController::mutatingUrl() const
{
    return mutating_url;
}

QString GifController::loadFolder(const QUrl &url)
{
    return library->loadFolder(url);
}

QString GifController::loadFolderUrl(const QUrl &url)
{
    return library->loadFolderUrl(url);
}

QString GifController::loadDefaultFolder()
{
    return library->loadDefaultFolder();
}

void GifController::setFilterQuery(const QString &query)
{
    library->setFilterQuery(query);
}

QString GifController::deleteFile(const QUrl &url)
{
    const QString error = storage->validateFile(url);
    if (!error.isEmpty())
        return error;

    beginFileMutation(url);

    const QString operation_error = storage->removeFile(url);
    finishFileMutation();
    return operation_error;
}

QString GifController::renameFile(const QUrl &url, const QString &new_base_name)
{
    const QString error = storage->validateFile(url);
    if (!error.isEmpty())
        return error;

    const GifStorage::RenamePlan plan = storage->planRename(url, new_base_name);
    if (!plan.error.isEmpty())
        return plan.error;
    if (!plan.changed)
        return {};

    beginFileMutation(url);

    const QString operation_error =
        storage->renameFile(url, plan.new_display_name);
    finishFileMutation();
    return operation_error;
}

void GifController::beginFileMutation(const QUrl &url)
{
    setMutatingUrl(url);
    emit fileAboutToChange(url);

    library->stopLoading();
    library->removeModelEntry(url);
}

void GifController::finishFileMutation()
{
    setMutatingUrl(QUrl());
    library->reloadCurrentFolder();
}

void GifController::applySettings()
{
    library->applySettings();
}

void GifController::shutdown()
{
    library->shutdown();
}

void GifController::setMutatingUrl(const QUrl &url)
{
    if (mutating_url == url)
        return;

    mutating_url = url;
    emit mutatingUrlChanged();
}
