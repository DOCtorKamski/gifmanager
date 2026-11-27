#include <QtTest>

#include "desktop_gif_storage.h"
#include "gif_item.h"
#include "gif_loader_worker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSet>
#include <QTemporaryDir>
#include <QUrl>

#include <memory>

namespace {

// Smallest well-known 1x1 GIF. Decoding it requires the Qt GIF plugin, but the
// tests only rely on the signal flow, so a missing plugin is not fatal.
QByteArray tinyGif()
{
    return QByteArray::fromHex(
        "47494638396101000100800000000000ffffff"
        "21f90401000000002c000000000100010000"
        "02024401003b");
}

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(contents) == contents.size();
}

std::shared_ptr<GifStorage> desktopStorage()
{
    return std::make_shared<DesktopGifStorage>();
}

// DesktopGifStorage with a forced listing failure, so the error path can be
// exercised without a real unreadable directory.
class FailingStorage : public DesktopGifStorage
{
public:
    Listing listGifs(const QUrl &, const StopPredicate &) override
    {
        Listing listing;
        listing.error = QStringLiteral("Folder cannot be read.");
        return listing;
    }
};

QList<GifItem> itemsFor(const QStringList &paths)
{
    QList<GifItem> items;
    for (const QString &path : paths)
        items.append({QUrl::fromLocalFile(path), QFileInfo(path).fileName()});
    return items;
}

} // namespace

class TestGifLoaderWorker : public QObject
{
    Q_OBJECT

private slots:
    void scansOnlyGifFiles();
    void scansGifFilesSortedByName();
    void reportsFilesBeforeThumbnails();
    void discoveryOnlySkipsThumbnails();
    void explicitFileListSkipsScan();
    void stopRequestBeforeProcessEmitsNoResults();
    void invalidImageProducesPlaceholder();
    void listingFailureEmitsDiscoveryFailed();
};

void TestGifLoaderWorker::scansOnlyGifFiles()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("b.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.GIF")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("note.txt")), "hello"));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("picture.png")), "not a png"));

    GifLoaderWorker worker(GifLoadRequest::fromContainer(
        QUrl::fromLocalFile(folder.path()), QSize(8, 8), desktopStorage()));
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 1);

    const QList<GifItem> items =
        discovered.first().at(0).value<QList<GifItem>>();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).fileName, QStringLiteral("a.GIF"));
    QCOMPARE(items.at(1).fileName, QStringLiteral("b.gif"));
    QVERIFY(items.at(0).url.isLocalFile());
    QVERIFY(QFileInfo(items.at(0).url.toLocalFile()).isAbsolute());

    QCOMPARE(loaded.count(), 2);
    QCOMPARE(finished.count(), 1);
}

void TestGifLoaderWorker::scansGifFilesSortedByName()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("z.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("m.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.gif")), tinyGif()));

    GifLoaderWorker worker(GifLoadRequest::fromContainer(
        QUrl::fromLocalFile(folder.path()), QSize(8, 8), desktopStorage()));
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);

    worker.process();

    const QList<GifItem> items =
        discovered.first().at(0).value<QList<GifItem>>();
    QCOMPARE(items.size(), 3);
    QCOMPARE(items.at(0).fileName, QStringLiteral("a.gif"));
    QCOMPARE(items.at(1).fileName, QStringLiteral("m.gif"));
    QCOMPARE(items.at(2).fileName, QStringLiteral("z.gif"));
}

void TestGifLoaderWorker::reportsFilesBeforeThumbnails()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("b.gif")), tinyGif()));

    GifLoaderWorker worker(GifLoadRequest::fromContainer(
        QUrl::fromLocalFile(folder.path()), QSize(8, 8), desktopStorage()));

    QStringList order;
    connect(&worker, &GifLoaderWorker::filesDiscovered, this,
            [&order] { order << QStringLiteral("discovered"); });
    connect(&worker, &GifLoaderWorker::gifLoaded, this,
            [&order] { order << QStringLiteral("loaded"); });
    connect(&worker, &GifLoaderWorker::progress, this,
            [&order] { order << QStringLiteral("progress"); });

    worker.process();

    QVERIFY(!order.isEmpty());
    QCOMPARE(order.first(), QStringLiteral("discovered"));
    QCOMPARE(order.count(QStringLiteral("loaded")), 2);
    QCOMPARE(order.count(QStringLiteral("progress")), 2);
    QVERIFY(order.last() == QStringLiteral("progress"));
}

void TestGifLoaderWorker::discoveryOnlySkipsThumbnails()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("b.gif")), tinyGif()));

    GifLoadRequest request = GifLoadRequest::fromContainer(
        QUrl::fromLocalFile(folder.path()), QSize(8, 8), desktopStorage());
    request.load_thumbnails = false;

    GifLoaderWorker worker(request);
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy progress(&worker, &GifLoaderWorker::progress);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 1);
    QCOMPARE(discovered.first().at(0).value<QList<GifItem>>().size(), 2);
    QCOMPARE(loaded.count(), 0);
    QCOMPARE(progress.count(), 0);
    QCOMPARE(finished.count(), 1);
}

void TestGifLoaderWorker::explicitFileListSkipsScan()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString first = folder.filePath(QStringLiteral("one.gif"));
    const QString second = folder.filePath(QStringLiteral("two.gif"));
    QVERIFY(writeFile(first, tinyGif()));
    QVERIFY(writeFile(second, tinyGif()));

    const QList<GifItem> files = itemsFor({first, second});
    GifLoaderWorker worker(GifLoadRequest::fromFiles(files, QSize(4, 4),
                                                     desktopStorage()));
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy progress(&worker, &GifLoaderWorker::progress);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 0);
    QCOMPARE(loaded.count(), 2);
    QCOMPARE(progress.count(), 2);
    QCOMPARE(progress.last().at(0).toInt(), 2);
    QCOMPARE(progress.last().at(1).toInt(), 2);
    // Thumbnails are decoded in parallel, so their completion order is not
    // guaranteed.
    QSet<QUrl> loadedUrls;
    for (const QList<QVariant> &signal : loaded)
        loadedUrls.insert(signal.at(0).toUrl());
    QCOMPARE(loadedUrls.size(), 2);
    QVERIFY(loadedUrls.contains(QUrl::fromLocalFile(first)));
    QVERIFY(loadedUrls.contains(QUrl::fromLocalFile(second)));
    QCOMPARE(finished.count(), 1);
}

void TestGifLoaderWorker::stopRequestBeforeProcessEmitsNoResults()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString file = folder.filePath(QStringLiteral("a.gif"));
    QVERIFY(writeFile(file, tinyGif()));

    GifLoaderWorker worker(GifLoadRequest::fromFiles(itemsFor({file}), QSize(8, 8),
                                                     desktopStorage()));
    worker.requestStop();

    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(loaded.count(), 0);
    QCOMPARE(finished.count(), 1);
}

void TestGifLoaderWorker::invalidImageProducesPlaceholder()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString broken = folder.filePath(QStringLiteral("broken.gif"));
    QVERIFY(writeFile(broken, "this is not a gif"));

    GifLoaderWorker worker(GifLoadRequest::fromFiles(itemsFor({broken}), QSize(12, 9),
                                                     desktopStorage()));
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);

    worker.process();

    QCOMPARE(loaded.count(), 1);
    QCOMPARE(loaded.first().at(1).value<QImage>().size(), QSize(12, 9));
}

void TestGifLoaderWorker::listingFailureEmitsDiscoveryFailed()
{
    GifLoaderWorker worker(GifLoadRequest::fromContainer(
        QUrl(QStringLiteral("file:///does/not/matter")), QSize(8, 8),
        std::make_shared<FailingStorage>()));
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy failed(&worker, &GifLoaderWorker::discoveryFailed);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 0);
    QCOMPARE(loaded.count(), 0);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(),
             QStringLiteral("Folder cannot be read."));
    QCOMPARE(finished.count(), 1);
}

QTEST_GUILESS_MAIN(TestGifLoaderWorker)

#include "tst_gif_loader_worker.moc"
