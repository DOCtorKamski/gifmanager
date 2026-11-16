#include <QtTest>

#include "desktop_gif_storage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(contents) == contents.size();
}

QUrl localUrl(const QString &path)
{
    return QUrl::fromLocalFile(path);
}

} // namespace

class TestDesktopGifStorage : public QObject
{
    Q_OBJECT

private slots:
    void validateMissingFileReturnsError();
    void removeFileDeletesExistingFile();
    void planRenamePreservesExtension();
    void planRenameSameNameIsNoOp();
    void planRenameRejectsEmptyName();
    void planRenameRejectsInvalidCharacters();
    void planRenameRejectsExistingTarget();
    void renameFileMovesFile();
};

void TestDesktopGifStorage::validateMissingFileReturnsError()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    DesktopGifStorage storage;
    const QString error = storage.validateFile(
        localUrl(folder.filePath(QStringLiteral("missing.gif"))));
    QVERIFY(!error.isEmpty());
}

void TestDesktopGifStorage::removeFileDeletesExistingFile()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    QVERIFY(storage.validateFile(localUrl(path)).isEmpty());
    QVERIFY(storage.removeFile(localUrl(path)).isEmpty());
    QVERIFY(!QFile::exists(path));
}

void TestDesktopGifStorage::planRenamePreservesExtension()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    const GifStorage::RenamePlan plan =
        storage.planRename(localUrl(path), QStringLiteral("renamed"));

    QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));
    QVERIFY(plan.changed);
    QCOMPARE(plan.new_display_name, QStringLiteral("renamed.gif"));
}

void TestDesktopGifStorage::planRenameSameNameIsNoOp()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    const GifStorage::RenamePlan plan =
        storage.planRename(localUrl(path), QStringLiteral("photo"));

    QVERIFY(plan.error.isEmpty());
    QVERIFY(!plan.changed);
    QVERIFY(plan.new_display_name.isEmpty());
}

void TestDesktopGifStorage::planRenameRejectsEmptyName()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    const GifStorage::RenamePlan plan =
        storage.planRename(localUrl(path), QStringLiteral("   "));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestDesktopGifStorage::planRenameRejectsInvalidCharacters()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    const GifStorage::RenamePlan plan =
        storage.planRename(localUrl(path), QStringLiteral("bad/name"));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestDesktopGifStorage::planRenameRejectsExistingTarget()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    const QString existing = folder.filePath(QStringLiteral("existing.gif"));
    QVERIFY(writeFile(path, "data"));
    QVERIFY(writeFile(existing, "data"));

    DesktopGifStorage storage;
    const GifStorage::RenamePlan plan =
        storage.planRename(localUrl(path), QStringLiteral("existing"));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestDesktopGifStorage::renameFileMovesFile()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    const QString new_path = folder.filePath(QStringLiteral("renamed.gif"));
    QVERIFY(writeFile(path, "data"));

    DesktopGifStorage storage;
    QVERIFY(storage.renameFile(localUrl(path),
                               QStringLiteral("renamed.gif")).isEmpty());
    QVERIFY(!QFile::exists(path));
    QVERIFY(QFile::exists(new_path));
}

QTEST_GUILESS_MAIN(TestDesktopGifStorage)

#include "tst_desktop_gif_storage.moc"
