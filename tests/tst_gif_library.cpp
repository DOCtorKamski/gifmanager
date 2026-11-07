#include <QtTest>

#include "gif_library.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QSettings>
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

class TestGifLibrary : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void loadsFolderAndReportsProgress();
    void stopLoadingFromLoadingChangedLeavesLibraryIdle();
    void reentrantLoadFromLoadingChangedKeepsLatestFolder();
    void reentrantLoadFromProgressChangedKeepsLatestFolder();
    void loadFolderReplacesPreviousLoad();
    void shutdownWhileLoadingStopsTask();

private:
    QTemporaryDir settings_dir;
};

void TestGifLibrary::initTestCase()
{
    QVERIFY(settings_dir.isValid());

    // Keep SettingsManager away from the real user configuration.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir.path());
}

void TestGifLibrary::loadsFolderAndReportsProgress()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 3).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    const QString error = library.loadFolder(folder.path());
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(library.isLoading());

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolder(), folder.path());
    QCOMPARE(library.progressCurrent(), 3);
    QCOMPARE(library.progressTotal(), 3);
    QCOMPARE(library.proxyModel()->rowCount(), 3);
}

void TestGifLibrary::stopLoadingFromLoadingChangedLeavesLibraryIdle()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 5).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    bool stopped = false;
    connect(&library, &GifLibrary::loadingChanged, &library, [&] {
        if (!stopped && library.isLoading()) {
            stopped = true;
            library.stopLoading();
        }
    });

    const QString error = library.loadFolder(folder.path());
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(stopped);
    QVERIFY(!library.isLoading());

    QTest::qWait(50);
    QVERIFY(!library.isLoading());
}

void TestGifLibrary::reentrantLoadFromLoadingChangedKeepsLatestFolder()
{
    QTemporaryDir first_folder;
    QTemporaryDir second_folder;
    QVERIFY(first_folder.isValid());
    QVERIFY(second_folder.isValid());
    QVERIFY(!writeGifs(first_folder, 2).isEmpty());
    QVERIFY(!writeGifs(second_folder, 3).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    bool switched = false;
    connect(&library, &GifLibrary::loadingChanged, &library, [&] {
        if (!switched && library.isLoading()) {
            switched = true;
            const QString error = library.loadFolder(second_folder.path());
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
    });

    const QString error = library.loadFolder(first_folder.path());
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(switched);

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolder(), second_folder.path());
    QCOMPARE(library.proxyModel()->rowCount(), 3);
}

void TestGifLibrary::reentrantLoadFromProgressChangedKeepsLatestFolder()
{
    QTemporaryDir first_folder;
    QTemporaryDir second_folder;
    QVERIFY(first_folder.isValid());
    QVERIFY(second_folder.isValid());
    QVERIFY(!writeGifs(first_folder, 2).isEmpty());
    QVERIFY(!writeGifs(second_folder, 3).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    bool switched = false;
    connect(&library, &GifLibrary::progressChanged, &library, [&] {
        if (!switched && library.isLoading()) {
            switched = true;
            const QString error = library.loadFolder(second_folder.path());
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
    });

    const QString error = library.loadFolder(first_folder.path());
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(switched);

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolder(), second_folder.path());
    QCOMPARE(library.proxyModel()->rowCount(), 3);
}

void TestGifLibrary::loadFolderReplacesPreviousLoad()
{
    QTemporaryDir first_folder;
    QTemporaryDir second_folder;
    QVERIFY(first_folder.isValid());
    QVERIFY(second_folder.isValid());
    QVERIFY(!writeGifs(first_folder, 4).isEmpty());
    QVERIFY(!writeGifs(second_folder, 2).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    QVERIFY(library.loadFolder(first_folder.path()).isEmpty());
    QVERIFY(library.loadFolder(second_folder.path()).isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolder(), second_folder.path());
    QCOMPARE(library.proxyModel()->rowCount(), 2);
}

void TestGifLibrary::shutdownWhileLoadingStopsTask()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 5).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    QVERIFY(library.loadFolder(folder.path()).isEmpty());
    QVERIFY(library.isLoading());

    library.shutdown();
    QVERIFY(!library.isLoading());

    // Shutdown must be idempotent.
    library.shutdown();
    QTest::qWait(50);
    QVERIFY(!library.isLoading());
}

QTEST_GUILESS_MAIN(TestGifLibrary)

#include "tst_gif_library.moc"
