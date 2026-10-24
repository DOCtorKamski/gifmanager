#ifndef GIF_LOADER_WORKER_H
#define GIF_LOADER_WORKER_H

#include <QAtomicInt>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>

struct GifLoadRequest
{
    enum class Source {
        Folder,
        FileList
    };

    Source source = Source::FileList;
    QString folder;
    QStringList files;
    QSize thumbnail_size;

    static GifLoadRequest fromFolder(const QString &folder_path,
                                     const QSize &thumbnail_size);
    static GifLoadRequest fromFiles(const QStringList &file_paths,
                                    const QSize &thumbnail_size);
};

class GifLoaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit GifLoaderWorker(GifLoadRequest request);

    void requestStop();

public slots:
    void process();

signals:
    void filesDiscovered(const QStringList &file_paths);
    void gifLoaded(const QString &file_path, const QImage &thumbnail);
    void progress(int current, int total);
    void finished();

private:
    QStringList scanFolder();
    bool shouldStop() const;

    GifLoadRequest request;
    QAtomicInt should_stop{0};
};

#endif // GIF_LOADER_WORKER_H
