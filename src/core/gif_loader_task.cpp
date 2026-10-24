#include "gif_loader_task.h"

#include <QDebug>
#include <QThread>

namespace {
constexpr int STOP_TIMEOUT_MS = 5000;
}

GifLoaderTask::GifLoaderTask(const GifLoadRequest &request_, QObject *parent)
    : QObject(parent)
    , request(request_)
{
}

GifLoaderTask::~GifLoaderTask()
{
    cancel();
}

void GifLoaderTask::start()
{
    if (started || cancelled)
        return;

    started = true;

    loader_thread = new QThread;
    worker = new GifLoaderWorker(request);
    worker->moveToThread(loader_thread);

    connect(loader_thread, &QThread::started,
            worker, &GifLoaderWorker::process);

    // The worker lives on its own thread, so these are queued connections. All
    // worker signals are emitted in order and therefore arrive in order.
    connect(worker, &GifLoaderWorker::filesDiscovered, this,
            [this](const QStringList &file_paths) {
                if (!cancelled)
                    emit filesDiscovered(file_paths);
            }, Qt::QueuedConnection);
    connect(worker, &GifLoaderWorker::gifLoaded, this,
            [this](const QString &file_path, const QImage &thumbnail) {
                if (!cancelled)
                    emit gifLoaded(file_path, thumbnail);
            }, Qt::QueuedConnection);
    connect(worker, &GifLoaderWorker::progress, this,
            [this](int current, int total) {
                if (!cancelled)
                    emit progress(current, total);
            }, Qt::QueuedConnection);
    connect(worker, &GifLoaderWorker::finished,
            this, &GifLoaderTask::onWorkerFinished, Qt::QueuedConnection);

    loader_thread->start();
}

void GifLoaderTask::cancel()
{
    cancelled = true;

    if (worker)
        worker->requestStop();

    teardownThread();
}

bool GifLoaderTask::isRunning() const
{
    return loader_thread && loader_thread->isRunning();
}

void GifLoaderTask::onWorkerFinished()
{
    if (!loader_thread)
        return;

    teardownThread();

    if (!cancelled)
        emit finished();
}

void GifLoaderTask::teardownThread()
{
    if (worker)
        disconnect(worker, nullptr, this, nullptr);

    if (loader_thread) {
        loader_thread->quit();
        if (!loader_thread->wait(STOP_TIMEOUT_MS)) {
            qWarning() << "Loader thread did not stop within timeout";
            loader_thread->terminate();
            loader_thread->wait();
        }
    }

    // The thread has stopped, so its worker can be destroyed from here.
    delete worker;
    worker = nullptr;

    delete loader_thread;
    loader_thread = nullptr;
}
