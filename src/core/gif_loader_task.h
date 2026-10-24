#ifndef GIF_LOADER_TASK_H
#define GIF_LOADER_TASK_H

#include "gif_loader_worker.h"

#include <QObject>

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
    void filesDiscovered(const QStringList &file_paths);
    void gifLoaded(const QString &file_path, const QImage &thumbnail);
    void progress(int current, int total);
    void finished();

private:
    void onWorkerFinished();
    void teardownThread();

    GifLoadRequest request;
    QThread *loader_thread = nullptr;
    GifLoaderWorker *worker = nullptr;
    bool cancelled = false;
    bool started = false;
};

#endif // GIF_LOADER_TASK_H
