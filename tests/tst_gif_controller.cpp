#include <QtTest>

#include "gif_controller.h"
#include "gif_model.h"
#include "gif_storage.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrl>

#include <atomic>

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

QStringList rowFileNames(QAbstractItemModel *model)
{
    QStringList names;
    for (int row = 0; row < model->rowCount(); ++row) {
        names.append(model->index(row, 0)
                         .data(GifModel::FileNameRole)
                         .toString());
    }
    return names;
}

class FakeGifStorage : public GifStorage
{
public:
    bool exists(const QUrl &) const override { return true; }

    QList<GifItem> listGifs(const QUrl &, const StopPredicate &) override
    {
        ++list_calls;
        return items;
    }

    QImage readThumbnail(const QUrl &, const QSize &) override { return {}; }

    QUrl playbackUrl(const QUrl &url) override
    {
        ++playback_calls;
        return playback_url;
    }

    QString validateFile(const QUrl &) const override
    {
        ++validate_calls;
        return validate_error;
    }

    RenamePlan planRename(const QUrl &, const QString &) const override
    {
        ++plan_calls;
        return plan;
    }

    QString removeFile(const QUrl &) override
    {
        ++remove_calls;
        return remove_error;
    }

    QString renameFile(const QUrl &, const QString &new_display_name) override
    {
        ++rename_calls;
        renamed_to = new_display_name;
        return rename_error;
    }

    QList<GifItem> items;
    RenamePlan plan;
    QString validate_error;
    QString remove_error;
    QString rename_error;
    QString renamed_to;
    QUrl playback_url;

    mutable std::atomic_int list_calls{0};
    mutable std::atomic_int validate_calls{0};
    mutable std::atomic_int plan_calls{0};
    mutable std::atomic_int remove_calls{0};
    mutable std::atomic_int rename_calls{0};
    mutable std::atomic_int playback_calls{0};
};

} // namespace

class TestGifController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void loadsFolderAndReportsProgress();
    void loadFolderReturnsErrorForMissingPath();
    void deleteFileRemovesRowAndReloads();
    void deleteMissingFileReturnsError();
    void renameFilePreservesExtensionAndReloads();
    void renameKeepsAlphabeticalOrder();
    void renameRejectsInvalidNameWithoutReloading();
    void reloadPicksUpExternalFiles();
    void mutationsAreDelegatedToStorage();
    void storageErrorsAreReturned();
    void playbackUrlIsDelegatedToStorage();

private:
    QTemporaryDir settings_dir;
};

void TestGifController::initTestCase()
{
    QVERIFY(settings_dir.isValid());

    // Keep SettingsManager away from the real user configuration.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir.path());
}

void TestGifController::loadsFolderAndReportsProgress()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    QVERIFY(!writeGifs(folder, 3).isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(controller.currentFolderUrl(), QUrl::fromLocalFile(folder.path()));
    QCOMPARE(controller.progressCurrent(), 3);
    QCOMPARE(controller.progressTotal(), 3);
    QCOMPARE(controller.proxyModel()->rowCount(), 3);
}

void TestGifController::loadFolderReturnsErrorForMissingPath()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString error = controller.loadFolder(
        QUrl::fromLocalFile(folder.filePath(QStringLiteral("missing"))));

    QVERIFY(!error.isEmpty());
    QVERIFY(!controller.isLoading());
    QVERIFY(controller.currentFolderUrl().isEmpty());
}

void TestGifController::deleteFileRemovesRowAndReloads()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 3);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 3);

    const QString victim = files.first();
    QCOMPARE(controller.deleteFile(QUrl::fromLocalFile(victim)), QString());
    QVERIFY(!QFile::exists(victim));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);
    QCOMPARE(controller.mutatingUrl(), QUrl());
}

void TestGifController::deleteMissingFileReturnsError()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString error = controller.deleteFile(
        QUrl::fromLocalFile(folder.filePath(QStringLiteral("missing.gif"))));

    QVERIFY(!error.isEmpty());
    QVERIFY(!controller.isLoading());
}

void TestGifController::renameFilePreservesExtensionAndReloads()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 2);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);

    const QString old_path = files.first();
    const QFileInfo old_info(old_path);
    const QString new_path = QDir(old_info.absolutePath())
                                 .filePath(QStringLiteral("renamed.gif"));

    const QString error = controller.renameFile(QUrl::fromLocalFile(old_path),
                                                QStringLiteral("renamed"));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QVERIFY(!QFile::exists(old_path));
    QVERIFY(QFile::exists(new_path));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);
    QCOMPARE(controller.mutatingUrl(), QUrl());
}

void TestGifController::renameKeepsAlphabeticalOrder()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 3);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(rowFileNames(controller.proxyModel()),
             QStringList({QStringLiteral("gif_000.gif"),
                          QStringLiteral("gif_001.gif"),
                          QStringLiteral("gif_002.gif")}));

    const QString error = controller.renameFile(QUrl::fromLocalFile(files.first()),
                                                QStringLiteral("zzz"));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(rowFileNames(controller.proxyModel()),
             QStringList({QStringLiteral("gif_001.gif"),
                          QStringLiteral("gif_002.gif"),
                          QStringLiteral("zzz.gif")}));
}

void TestGifController::reloadPicksUpExternalFiles()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 2);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    const QString external_path = folder.filePath(QStringLiteral("extra.gif"));
    QVERIFY(writeFile(external_path, tinyGif()));

    const QString error = controller.deleteFile(QUrl::fromLocalFile(files.first()));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(rowFileNames(controller.proxyModel()),
             QStringList({QStringLiteral("extra.gif"),
                          QStringLiteral("gif_001.gif")}));
}

void TestGifController::renameRejectsInvalidNameWithoutReloading()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 1);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error =
        controller.loadFolder(QUrl::fromLocalFile(folder.path()));
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    const QString path = files.first();
    const QString error = controller.renameFile(QUrl::fromLocalFile(path),
                                                QStringLiteral("bad/name"));

    QVERIFY(!error.isEmpty());
    QVERIFY(!controller.isLoading());
    QVERIFY(QFile::exists(path));
    QCOMPARE(controller.proxyModel()->rowCount(), 1);
}

void TestGifController::mutationsAreDelegatedToStorage()
{
    const QUrl container(QStringLiteral("content://test/tree"));
    const QUrl file(QStringLiteral("content://test/tree/document/1"));

    auto storage = std::make_shared<FakeGifStorage>();
    storage->items = {{file, QStringLiteral("photo.gif")}};
    storage->plan = {{}, QStringLiteral("renamed.gif"), true};

    ThumbnailProvider provider;
    GifController controller(&provider, storage);

    const QString load_error = controller.loadFolder(container);
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 1);
    QCOMPARE(storage->list_calls.load(), 1);

    QCOMPARE(controller.deleteFile(file), QString());
    QCOMPARE(storage->validate_calls.load(), 1);
    QCOMPARE(storage->remove_calls.load(), 1);

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(controller.renameFile(file, QStringLiteral("renamed")), QString());
    QCOMPARE(storage->validate_calls.load(), 2);
    QCOMPARE(storage->plan_calls.load(), 1);
    QCOMPARE(storage->rename_calls.load(), 1);
    QCOMPARE(storage->renamed_to, QStringLiteral("renamed.gif"));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
}

void TestGifController::storageErrorsAreReturned()
{
    const QUrl container(QStringLiteral("content://test/tree"));
    const QUrl file(QStringLiteral("content://test/tree/document/1"));

    auto storage = std::make_shared<FakeGifStorage>();
    storage->items = {{file, QStringLiteral("photo.gif")}};
    storage->plan = {{}, QStringLiteral("renamed.gif"), true};
    storage->remove_error = QStringLiteral("delete failed");
    storage->rename_error = QStringLiteral("rename failed");

    ThumbnailProvider provider;
    GifController controller(&provider, storage);

    const QString load_error = controller.loadFolder(container);
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(controller.deleteFile(file), QStringLiteral("delete failed"));
    QCOMPARE(storage->remove_calls.load(), 1);
    QCOMPARE(controller.mutatingUrl(), QUrl());

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(controller.renameFile(file, QStringLiteral("renamed")),
             QStringLiteral("rename failed"));
    QCOMPARE(storage->rename_calls.load(), 1);
    QCOMPARE(controller.mutatingUrl(), QUrl());
}

void TestGifController::playbackUrlIsDelegatedToStorage()
{
    const QUrl container(QStringLiteral("content://test/tree"));
    const QUrl file(QStringLiteral("content://test/tree/document/1"));

    auto storage = std::make_shared<FakeGifStorage>();
    const QUrl cached(QStringLiteral("file:///data/cache/playback/x.gif"));
    storage->playback_url = cached;

    ThumbnailProvider provider;
    GifController controller(&provider, storage);

    const QUrl result = controller.playbackUrl(file);

    QCOMPARE(storage->playback_calls.load(), 1);
    QCOMPARE(result, cached);
}

QTEST_GUILESS_MAIN(TestGifController)

#include "tst_gif_controller.moc"
