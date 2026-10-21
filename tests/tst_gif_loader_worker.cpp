#include <QtTest>

#include "gif_loader_worker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

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

} // namespace

class TestGifLoaderWorker : public QObject
{
    Q_OBJECT

private slots:
    void scansOnlyGifFiles();
    void scansGifFilesSortedByName();
    void reportsFilesBeforeThumbnails();
    void explicitFileListSkipsScan();
    void stopRequestBeforeProcessEmitsNoResults();
    void invalidImageProducesPlaceholder();
};

void TestGifLoaderWorker::scansOnlyGifFiles()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("b.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.GIF")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("note.txt")), "hello"));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("picture.png")), "not a png"));

    GifLoaderWorker worker(folder.path(), QSize(8, 8), 7);
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 1);
    QCOMPARE(discovered.first().at(0).toULongLong(), 7ULL);

    const QStringList files = discovered.first().at(1).toStringList();
    QCOMPARE(files.size(), 2);
    QCOMPARE(QFileInfo(files.at(0)).fileName(), QStringLiteral("a.GIF"));
    QCOMPARE(QFileInfo(files.at(1)).fileName(), QStringLiteral("b.gif"));
    QVERIFY(QFileInfo(files.at(0)).isAbsolute());

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

    GifLoaderWorker worker(folder.path(), QSize(8, 8), 1);
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);

    worker.process();

    const QStringList files = discovered.first().at(1).toStringList();
    QCOMPARE(files.size(), 3);
    QCOMPARE(QFileInfo(files.at(0)).fileName(), QStringLiteral("a.gif"));
    QCOMPARE(QFileInfo(files.at(1)).fileName(), QStringLiteral("m.gif"));
    QCOMPARE(QFileInfo(files.at(2)).fileName(), QStringLiteral("z.gif"));
}

void TestGifLoaderWorker::reportsFilesBeforeThumbnails()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("a.gif")), tinyGif()));
    QVERIFY(writeFile(folder.filePath(QStringLiteral("b.gif")), tinyGif()));

    GifLoaderWorker worker(folder.path(), QSize(8, 8), 2);

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

void TestGifLoaderWorker::explicitFileListSkipsScan()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString first = folder.filePath(QStringLiteral("one.gif"));
    const QString second = folder.filePath(QStringLiteral("two.gif"));
    QVERIFY(writeFile(first, tinyGif()));
    QVERIFY(writeFile(second, tinyGif()));

    const QStringList files{first, second};
    GifLoaderWorker worker(files, QSize(4, 4), 5);
    QSignalSpy discovered(&worker, &GifLoaderWorker::filesDiscovered);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy progress(&worker, &GifLoaderWorker::progress);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(discovered.count(), 0);
    QCOMPARE(loaded.count(), 2);
    QCOMPARE(progress.count(), 2);
    QCOMPARE(progress.last().at(1).toInt(), 2);
    QCOMPARE(progress.last().at(2).toInt(), 2);
    QCOMPARE(loaded.last().at(1).toString(), second);
    QCOMPARE(finished.count(), 1);
}

void TestGifLoaderWorker::stopRequestBeforeProcessEmitsNoResults()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString file = folder.filePath(QStringLiteral("a.gif"));
    QVERIFY(writeFile(file, tinyGif()));

    GifLoaderWorker worker(QStringList{file}, QSize(8, 8), 3);
    worker.requestStop();

    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);
    QSignalSpy finished(&worker, &GifLoaderWorker::finished);

    worker.process();

    QCOMPARE(loaded.count(), 0);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().at(0).toULongLong(), 3ULL);
}

void TestGifLoaderWorker::invalidImageProducesPlaceholder()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString broken = folder.filePath(QStringLiteral("broken.gif"));
    QVERIFY(writeFile(broken, "this is not a gif"));

    GifLoaderWorker worker(QStringList{broken}, QSize(12, 9), 4);
    QSignalSpy loaded(&worker, &GifLoaderWorker::gifLoaded);

    worker.process();

    QCOMPARE(loaded.count(), 1);
    QCOMPARE(loaded.first().at(2).value<QImage>().size(), QSize(12, 9));
}

QTEST_GUILESS_MAIN(TestGifLoaderWorker)

#include "tst_gif_loader_worker.moc"
