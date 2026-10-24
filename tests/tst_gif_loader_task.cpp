#include <QtTest>

#include "gif_loader_task.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>

namespace {

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

QStringList writeGifs(QTemporaryDir &folder, int count)
{
    QStringList paths;
    for (int i = 0; i < count; ++i) {
        const QString path = folder.filePath(
            QStringLiteral("gif_%1.gif").arg(i, 3, 10, QLatin1Char('0')));
        if (!writeFile(path, tinyGif()))
            return {};
        paths.append(path);
    }
    return paths;
}

} // namespace

class TestGifLoaderTask : public QObject
{
    Q_OBJECT

private slots:
    void loadsFolderAndEmitsOrderedSignals();
    void explicitFileListSkipsScan();
    void cancelDoesNotEmitFinished();
    void destructorStopsRunningTask();
};

void TestGifLoaderTask::loadsFolderAndEmitsOrderedSignals()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 3).isEmpty());

    GifLoaderTask task(GifLoadRequest::fromFolder(folder.path(), QSize(8, 8)));

    QStringList order;
    QSignalSpy discovered(&task, &GifLoaderTask::filesDiscovered);
    QSignalSpy loaded(&task, &GifLoaderTask::gifLoaded);
    QSignalSpy progress(&task, &GifLoaderTask::progress);
    QSignalSpy finished(&task, &GifLoaderTask::finished);

    connect(&task, &GifLoaderTask::filesDiscovered, this,
            [&order] { order << QStringLiteral("discovered"); });
    connect(&task, &GifLoaderTask::gifLoaded, this,
            [&order] { order << QStringLiteral("loaded"); });
    connect(&task, &GifLoaderTask::progress, this,
            [&order] { order << QStringLiteral("progress"); });
    connect(&task, &GifLoaderTask::finished, this,
            [&order] { order << QStringLiteral("finished"); });

    task.start();
    QVERIFY(finished.wait(5000));

    QCOMPARE(discovered.count(), 1);
    QCOMPARE(discovered.first().at(0).toStringList().size(), 3);
    QCOMPARE(loaded.count(), 3);
    QCOMPARE(progress.count(), 3);
    QCOMPARE(progress.last().at(0).toInt(), 3);
    QCOMPARE(progress.last().at(1).toInt(), 3);
    QVERIFY(!task.isRunning());

    QCOMPARE(order.first(), QStringLiteral("discovered"));
    QCOMPARE(order.last(), QStringLiteral("finished"));
    QCOMPARE(order.count(QStringLiteral("loaded")), 3);
    QCOMPARE(order.count(QStringLiteral("progress")), 3);
}

void TestGifLoaderTask::explicitFileListSkipsScan()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 2);
    QVERIFY(!files.isEmpty());

    GifLoaderTask task(GifLoadRequest::fromFiles(files, QSize(8, 8)));

    QSignalSpy discovered(&task, &GifLoaderTask::filesDiscovered);
    QSignalSpy loaded(&task, &GifLoaderTask::gifLoaded);
    QSignalSpy finished(&task, &GifLoaderTask::finished);

    task.start();
    QVERIFY(finished.wait(5000));

    QCOMPARE(discovered.count(), 0);
    QCOMPARE(loaded.count(), 2);
    QCOMPARE(loaded.last().at(0).toString(), files.last());
}

void TestGifLoaderTask::cancelDoesNotEmitFinished()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 50).isEmpty());

    GifLoaderTask task(GifLoadRequest::fromFolder(folder.path(), QSize(8, 8)));

    QSignalSpy finished(&task, &GifLoaderTask::finished);
    QSignalSpy loaded(&task, &GifLoaderTask::gifLoaded);

    task.start();
    task.cancel();

    QVERIFY(!task.isRunning());

    QTest::qWait(50);

    QCOMPARE(finished.count(), 0);
    QCOMPARE(loaded.count(), 0);
}

void TestGifLoaderTask::destructorStopsRunningTask()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 20).isEmpty());

    {
        GifLoaderTask task(GifLoadRequest::fromFolder(folder.path(), QSize(8, 8)));
        QSignalSpy loaded(&task, &GifLoaderTask::gifLoaded);
        task.start();
        QVERIFY(loaded.wait(5000));
    }

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(TestGifLoaderTask)

#include "tst_gif_loader_task.moc"
