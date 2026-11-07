#include <QtTest>

#include "gif_controller.h"
#include "gif_model.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    const QString error = controller.loadFolder(folder.path());
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(controller.currentFolder(), folder.path());
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

    const QString error = controller.loadFolder(folder.filePath(QStringLiteral("missing")));

    QVERIFY(!error.isEmpty());
    QVERIFY(!controller.isLoading());
    QVERIFY(controller.currentFolder().isEmpty());
}

void TestGifController::deleteFileRemovesRowAndReloads()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 3);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error = controller.loadFolder(folder.path());
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 3);

    const QString victim = files.first();
    QCOMPARE(controller.deleteFile(victim), QString());
    QVERIFY(!QFile::exists(victim));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);
    QCOMPARE(controller.mutatingPath(), QString());
}

void TestGifController::deleteMissingFileReturnsError()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString error = controller.deleteFile(
        folder.filePath(QStringLiteral("missing.gif")));

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

    const QString load_error = controller.loadFolder(folder.path());
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);

    const QString old_path = files.first();
    const QFileInfo old_info(old_path);
    const QString new_path = QDir(old_info.absolutePath())
                                 .filePath(QStringLiteral("renamed.gif"));

    const QString error = controller.renameFile(old_path, QStringLiteral("renamed"));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QVERIFY(!QFile::exists(old_path));
    QVERIFY(QFile::exists(new_path));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);
    QCOMPARE(controller.mutatingPath(), QString());
}

void TestGifController::renameKeepsAlphabeticalOrder()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 3);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    const QString load_error = controller.loadFolder(folder.path());
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(rowFileNames(controller.proxyModel()),
             QStringList({QStringLiteral("gif_000.gif"),
                          QStringLiteral("gif_001.gif"),
                          QStringLiteral("gif_002.gif")}));

    const QString error = controller.renameFile(files.first(),
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

    const QString load_error = controller.loadFolder(folder.path());
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    const QString external_path = folder.filePath(QStringLiteral("extra.gif"));
    QVERIFY(writeFile(external_path, tinyGif()));

    const QString error = controller.deleteFile(files.first());
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

    const QString load_error = controller.loadFolder(folder.path());
    QVERIFY2(load_error.isEmpty(), qPrintable(load_error));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    const QString path = files.first();
    const QString error = controller.renameFile(path, QStringLiteral("bad/name"));

    QVERIFY(!error.isEmpty());
    QVERIFY(!controller.isLoading());
    QVERIFY(QFile::exists(path));
    QCOMPARE(controller.proxyModel()->rowCount(), 1);
}

QTEST_GUILESS_MAIN(TestGifController)

#include "tst_gif_controller.moc"
