#include <QtTest>

#include "playback_cache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

class TestPlaybackCache : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void filePathIsStableAndUnique();
    void emptyCacheReturnsNoUrl();
    void storeWritesFileAndReturnsUrl();
    void storeKeepsExistingCopy();
    void storeRejectsEmptyData();
    void clearRemovesCachedFiles();
};

void TestPlaybackCache::initTestCase()
{
    // Keep the cache inside a throwaway directory instead of the real user
    // cache location.
    QStandardPaths::setTestModeEnabled(true);
}

void TestPlaybackCache::init()
{
    // The constructor clears the previous session's files.
    PlaybackCache cache;
}

void TestPlaybackCache::filePathIsStableAndUnique()
{
    const QUrl first(QStringLiteral("content://provider/tree/a/document/a%3Aone.gif"));
    const QUrl second(QStringLiteral("content://provider/tree/a/document/a%3Atwo.gif"));

    QCOMPARE(PlaybackCache::filePath(first), PlaybackCache::filePath(first));
    QVERIFY(PlaybackCache::filePath(first) != PlaybackCache::filePath(second));

    const QFileInfo path(PlaybackCache::filePath(first));
    QCOMPARE(path.absolutePath(), PlaybackCache::directory());
    QCOMPARE(path.suffix(), QStringLiteral("gif"));
}

void TestPlaybackCache::emptyCacheReturnsNoUrl()
{
    PlaybackCache cache;
    QVERIFY(cache.cachedUrl(QUrl(QStringLiteral("content://provider/document/missing.gif")))
                .isEmpty());
}

void TestPlaybackCache::storeWritesFileAndReturnsUrl()
{
    PlaybackCache cache;
    const QUrl source(QStringLiteral("content://provider/tree/a/document/a%3Aone.gif"));
    const QByteArray data = QByteArray::fromHex("474946383961");

    const QUrl stored = cache.store(source, data);
    QVERIFY(!stored.isEmpty());
    QVERIFY(stored.isLocalFile());
    QCOMPARE(stored, QUrl::fromLocalFile(PlaybackCache::filePath(source)));
    QCOMPARE(cache.cachedUrl(source), stored);

    QFile file(stored.toLocalFile());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
}

void TestPlaybackCache::storeKeepsExistingCopy()
{
    PlaybackCache cache;
    const QUrl source(QStringLiteral("content://provider/tree/a/document/a%3Aone.gif"));

    const QUrl first = cache.store(source, QByteArrayLiteral("first"));
    QVERIFY(!first.isEmpty());
    const QUrl second = cache.store(source, QByteArrayLiteral("second"));

    QCOMPARE(second, first);

    QFile file(second.toLocalFile());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArrayLiteral("first"));
}

void TestPlaybackCache::storeRejectsEmptyData()
{
    PlaybackCache cache;
    const QUrl source(QStringLiteral("content://provider/tree/a/document/a%3Aone.gif"));

    QVERIFY(cache.store(source, QByteArray()).isEmpty());
    QVERIFY(cache.cachedUrl(source).isEmpty());
}

void TestPlaybackCache::clearRemovesCachedFiles()
{
    PlaybackCache cache;
    const QUrl source(QStringLiteral("content://provider/tree/a/document/a%3Aone.gif"));
    const QUrl stored = cache.store(source, QByteArrayLiteral("data"));
    QVERIFY(!stored.isEmpty());

    cache.clear();

    QVERIFY(cache.cachedUrl(source).isEmpty());
    QVERIFY(!QFileInfo::exists(stored.toLocalFile()));
}

QTEST_GUILESS_MAIN(TestPlaybackCache)

#include "tst_playback_cache.moc"
