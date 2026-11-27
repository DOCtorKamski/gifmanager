#include <QtTest>

#include "desktop_gif_storage.h"
#include "gif_thumbnail_service.h"

#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

#include <memory>

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

// Delays thumbnail reads so duplicate requests and clear() can be observed
// before the first job finishes.
class SlowStorage : public DesktopGifStorage
{
public:
    QImage readThumbnail(const QUrl &url, const QSize &size) override
    {
        QThread::msleep(50);
        return DesktopGifStorage::readThumbnail(url, size);
    }
};

class NullStorage : public DesktopGifStorage
{
public:
    QImage readThumbnail(const QUrl &, const QSize &) override { return {}; }
};

} // namespace

class TestGifThumbnailService : public QObject
{
    Q_OBJECT

private slots:
    void loadsRequestedThumbnail();
    void deduplicatesPendingRequests();
    void clearDropsInFlightResults();
    void missingThumbnailBecomesPlaceholder();
};

void TestGifThumbnailService::loadsRequestedThumbnail()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString path = folder.filePath(QStringLiteral("a.gif"));
    QVERIFY(writeFile(path, tinyGif()));

    auto storage = std::make_shared<DesktopGifStorage>();
    GifThumbnailService service(storage);
    service.setThumbnailSize(QSize(16, 16));

    QSignalSpy spy(&service, &GifThumbnailService::thumbnailReady);
    const QUrl url = QUrl::fromLocalFile(path);
    service.request(url);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toUrl(), url);

    const QImage image = args.at(1).value<QImage>();
    QVERIFY(!image.isNull());
    QVERIFY(image.width() <= 16);
    QVERIFY(image.height() <= 16);
}

void TestGifThumbnailService::deduplicatesPendingRequests()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString path = folder.filePath(QStringLiteral("a.gif"));
    QVERIFY(writeFile(path, tinyGif()));

    auto storage = std::make_shared<SlowStorage>();
    GifThumbnailService service(storage);
    service.setThumbnailSize(QSize(8, 8));

    QSignalSpy spy(&service, &GifThumbnailService::thumbnailReady);
    const QUrl url = QUrl::fromLocalFile(path);
    service.request(url);
    service.request(url);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
    QTest::qWait(100);
    QCOMPARE(spy.count(), 1);
}

void TestGifThumbnailService::clearDropsInFlightResults()
{
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString path = folder.filePath(QStringLiteral("a.gif"));
    QVERIFY(writeFile(path, tinyGif()));

    auto storage = std::make_shared<SlowStorage>();
    GifThumbnailService service(storage);
    service.setThumbnailSize(QSize(8, 8));

    QSignalSpy spy(&service, &GifThumbnailService::thumbnailReady);
    service.request(QUrl::fromLocalFile(path));
    service.clear();

    QTest::qWait(200);
    QCOMPARE(spy.count(), 0);
}

void TestGifThumbnailService::missingThumbnailBecomesPlaceholder()
{
    auto storage = std::make_shared<NullStorage>();
    GifThumbnailService service(storage);
    service.setThumbnailSize(QSize(12, 9));

    QSignalSpy spy(&service, &GifThumbnailService::thumbnailReady);
    service.request(QUrl(QStringLiteral("file:///does-not-exist.gif")));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
    const QImage image = spy.takeFirst().at(1).value<QImage>();
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), QSize(12, 9));
}

QTEST_GUILESS_MAIN(TestGifThumbnailService)

#include "tst_gif_thumbnail_service.moc"
