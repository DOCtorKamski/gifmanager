#ifndef GIF_LOADER_TASK_H
#define GIF_LOADER_TASK_H

#include "gif_loader_worker.h"

#include <QObject>
#include <QUrl>

#include <memory>

class QThread;

class GifLoaderTask : public QObject
{
    Q_OBJECT
public:
    explicit GifLoaderTask(const GifLoadRequest &request, QObject *parent = nullptr);
    ~GifLoaderTask() override;

    void start();
    void cancel();
    bool isRunning() const;

signals:
    void filesDiscovered(const QList<GifItem> &items);
    void discoveryFailed(const QString &error);
    void gifLoaded(const QUrl &url, const QImage &thumbnail);
    void progress(int current, int total);
    void finished();

private:
    void onWorkerFinished();
    void teardownThread();

    GifLoadRequest request;
    std::unique_ptr<QThread> loader_thread;
    std::unique_ptr<GifLoaderWorker> worker;
    bool cancelled = false;
    bool started = false;
};

#endif // GIF_LOADER_TASK_H
