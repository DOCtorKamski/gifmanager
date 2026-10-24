#include <QtTest>

#include "gif_controller.h"
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

class TestGifController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void loadsFolderAndReportsProgress();
    void deleteFileRemovesRowAndReloads();

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

    QSignalSpy errors(&controller, &GifController::operationError);

    controller.loadFolder(folder.path());

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);

    QCOMPARE(errors.count(), 0);
    QCOMPARE(controller.currentFolder(), folder.path());
    QCOMPARE(controller.progressCurrent(), 3);
    QCOMPARE(controller.progressTotal(), 3);
    QCOMPARE(controller.proxyModel()->rowCount(), 3);
}

void TestGifController::deleteFileRemovesRowAndReloads()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QStringList files = writeGifs(folder, 3);
    QVERIFY(!files.isEmpty());

    ThumbnailProvider provider;
    GifController controller(&provider);

    controller.loadFolder(folder.path());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 3);

    const QString victim = files.first();
    QCOMPARE(controller.deleteFile(victim), QString());
    QVERIFY(!QFile::exists(victim));

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isLoading(), 10000);
    QCOMPARE(controller.proxyModel()->rowCount(), 2);
}

QTEST_GUILESS_MAIN(TestGifController)

#include "tst_gif_controller.moc"
