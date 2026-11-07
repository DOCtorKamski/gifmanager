#include "gif_controller.h"

#include "gif_file_operations.h"
#include "gif_library.h"

GifController::GifController(ThumbnailProvider *provider, QObject *parent)
    : QObject(parent)
    , library(new GifLibrary(provider, this))
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

QString GifController::currentFolder() const
{
    return library->currentFolder();
}

QUrl GifController::currentFolderUrl() const
{
    return library->currentFolderUrl();
}

QString GifController::mutatingPath() const
{
    return mutating_path;
}

QString GifController::loadFolder(const QString &path)
{
    return library->loadFolder(path);
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

QString GifController::deleteFile(const QString &path)
{
    const QString error = GifFileOperations::validateFile(path);
    if (!error.isEmpty())
        return error;

    beginFileMutation(path);

    const QString operation_error = GifFileOperations::removeFile(path);
    finishFileMutation();
    return operation_error;
}

QString GifController::renameFile(const QString &path, const QString &new_base_name)
{
    const QString error = GifFileOperations::validateFile(path);
    if (!error.isEmpty())
        return error;

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, new_base_name);
    if (!plan.error.isEmpty())
        return plan.error;
    if (!plan.changed)
        return {};

    beginFileMutation(path);

    const QString operation_error =
        GifFileOperations::renameFile(path, plan.new_path);
    finishFileMutation();
    return operation_error;
}

void GifController::beginFileMutation(const QString &path)
{
    setMutatingPath(path);
    emit fileAboutToChange(path);

    library->stopLoading();
    library->removeModelEntry(path);
}

void GifController::finishFileMutation()
{
    setMutatingPath(QString());
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

void GifController::setMutatingPath(const QString &path)
{
    if (mutating_path == path)
        return;

    mutating_path = path;
    emit mutatingPathChanged();
}
