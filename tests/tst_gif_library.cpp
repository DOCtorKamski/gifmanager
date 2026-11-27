#include <QtTest>

#include "desktop_gif_storage.h"
#include "gif_library.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QPersistentModelIndex>
#include <QSettings>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

#include <memory>

namespace {

// DesktopGifStorage with a forced listing failure so the asynchronous error
// path can be tested without an unreadable directory.
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

// Keeps the discovery task alive long enough for shutdown tests to observe an
// active load now that production no longer waits for thumbnails.
class SlowStorage : public DesktopGifStorage
{
public:
    Listing listGifs(const QUrl &container, const StopPredicate &shouldStop) override
    {
        QThread::msleep(50);
        return DesktopGifStorage::listGifs(container, shouldStop);
    }
};

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
    void requestThumbnailPopulatesModel();
    void stopLoadingFromLoadingChangedLeavesLibraryIdle();
    void reentrantLoadFromLoadingChangedKeepsLatestFolder();
    void reentrantLoadFromProgressChangedKeepsLatestFolder();
    void loadFolderReplacesPreviousLoad();
    void shutdownWhileLoadingStopsTask();
    void listingFailureEmitsLoadFailed();

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

    const QString error = library.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(library.isLoading());

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolderUrl(), QUrl::fromLocalFile(folder.path()));
    QCOMPARE(library.progressCurrent(), 3);
    QCOMPARE(library.progressTotal(), 3);
    QCOMPARE(library.proxyModel()->rowCount(), 3);
}

void TestGifLibrary::requestThumbnailPopulatesModel()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 2).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider);

    QVERIFY(library.loadFolder(QUrl::fromLocalFile(folder.path())).isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QAbstractItemModel *model = library.proxyModel();
    QCOMPARE(model->rowCount(), 2);

    const QPersistentModelIndex index(model->index(0, 0));
    QVERIFY(!index.data(GifModel::ThumbnailReadyRole).toBool());
    const QUrl url = index.data(GifModel::FileUrlRole).toUrl();

    library.requestThumbnail(url);
    QTRY_VERIFY_WITH_TIMEOUT(
        index.data(GifModel::ThumbnailReadyRole).toBool(), 5000);
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

    const QString error = library.loadFolder(QUrl::fromLocalFile(folder.path()));
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
            const QString error =
                library.loadFolder(QUrl::fromLocalFile(second_folder.path()));
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
    });

    const QString error = library.loadFolder(QUrl::fromLocalFile(first_folder.path()));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(switched);

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolderUrl(),
             QUrl::fromLocalFile(second_folder.path()));
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
            const QString error =
                library.loadFolder(QUrl::fromLocalFile(second_folder.path()));
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
    });

    const QString error = library.loadFolder(QUrl::fromLocalFile(first_folder.path()));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(switched);

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolderUrl(),
             QUrl::fromLocalFile(second_folder.path()));
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

    QVERIFY(library.loadFolder(QUrl::fromLocalFile(first_folder.path())).isEmpty());
    QVERIFY(library.loadFolder(QUrl::fromLocalFile(second_folder.path())).isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(library.currentFolderUrl(),
             QUrl::fromLocalFile(second_folder.path()));
    QCOMPARE(library.proxyModel()->rowCount(), 2);
}

void TestGifLibrary::shutdownWhileLoadingStopsTask()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 5).isEmpty());

    ThumbnailProvider provider;
    GifLibrary library(&provider, std::make_shared<SlowStorage>());

    QVERIFY(library.loadFolder(QUrl::fromLocalFile(folder.path())).isEmpty());
    QVERIFY(library.isLoading());

    library.shutdown();
    QVERIFY(!library.isLoading());

    // Shutdown must be idempotent.
    library.shutdown();
    QTest::qWait(50);
    QVERIFY(!library.isLoading());
}

void TestGifLibrary::listingFailureEmitsLoadFailed()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    ThumbnailProvider provider;
    GifLibrary library(&provider, std::make_shared<FailingStorage>());

    QSignalSpy failed(&library, &GifLibrary::loadFailed);

    const QString error = library.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(library.isLoading());

    QTRY_VERIFY_WITH_TIMEOUT(!library.isLoading(), 10000);

    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(),
             QStringLiteral("Folder cannot be read."));
    QCOMPARE(library.proxyModel()->rowCount(), 0);
}

QTEST_GUILESS_MAIN(TestGifLibrary)

#include "tst_gif_library.moc"
