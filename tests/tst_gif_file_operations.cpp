#include <QtTest>

#include "gif_file_operations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(contents) == contents.size();
}

} // namespace

class TestGifFileOperations : public QObject
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

void TestGifFileOperations::validateMissingFileReturnsError()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString error = GifFileOperations::validateFile(
        folder.filePath(QStringLiteral("missing.gif")));
    QVERIFY(!error.isEmpty());
}

void TestGifFileOperations::removeFileDeletesExistingFile()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    QVERIFY(GifFileOperations::validateFile(path).isEmpty());
    QVERIFY(GifFileOperations::removeFile(path).isEmpty());
    QVERIFY(!QFile::exists(path));
}

void TestGifFileOperations::planRenamePreservesExtension()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, QStringLiteral("renamed"));

    QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));
    QVERIFY(plan.changed);
    QCOMPARE(plan.new_path, folder.filePath(QStringLiteral("renamed.gif")));
}

void TestGifFileOperations::planRenameSameNameIsNoOp()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, QStringLiteral("photo"));

    QVERIFY(plan.error.isEmpty());
    QVERIFY(!plan.changed);
    QVERIFY(plan.new_path.isEmpty());
}

void TestGifFileOperations::planRenameRejectsEmptyName()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, QStringLiteral("   "));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestGifFileOperations::planRenameRejectsInvalidCharacters()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    QVERIFY(writeFile(path, "data"));

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, QStringLiteral("bad/name"));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestGifFileOperations::planRenameRejectsExistingTarget()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    const QString existing = folder.filePath(QStringLiteral("existing.gif"));
    QVERIFY(writeFile(path, "data"));
    QVERIFY(writeFile(existing, "data"));

    const GifFileOperations::RenamePlan plan =
        GifFileOperations::planRename(path, QStringLiteral("existing"));

    QVERIFY(!plan.error.isEmpty());
    QVERIFY(!plan.changed);
}

void TestGifFileOperations::renameFileMovesFile()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QString path = folder.filePath(QStringLiteral("photo.gif"));
    const QString new_path = folder.filePath(QStringLiteral("renamed.gif"));
    QVERIFY(writeFile(path, "data"));

    QVERIFY(GifFileOperations::renameFile(path, new_path).isEmpty());
    QVERIFY(!QFile::exists(path));
    QVERIFY(QFile::exists(new_path));
}

QTEST_GUILESS_MAIN(TestGifFileOperations)

#include "tst_gif_file_operations.moc"
