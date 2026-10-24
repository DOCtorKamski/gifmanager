#include <QtTest>

#include "gif_model.h"
#include "thumbnail_provider.h"

#include <QColor>
#include <QImage>
#include <QUrl>

class TestGifModel : public QObject
{
    Q_OBJECT

private slots:
    void roleNamesMatchTheQmlContract();
    void resetFilesPopulatesRows();
    void invalidIndexReturnsNothing();
    void thumbnailStartsHidden();
    void setThumbnailPublishesImage();
    void thumbnailRevisionChangesUrl();
    void thumbnailIdChangesAcrossResets();
    void removeFileShrinksModel();
    void removeUnknownFileReturnsFalse();
    void resetFilesClearsProvider();

private:
    static QString imageIdFromUrl(const QUrl &url);
    static QImage solidImage(int width, int height, const QColor &color);

    ThumbnailProvider provider;
    GifModel model{&provider};
};

QString TestGifModel::imageIdFromUrl(const QUrl &url)
{
    const QString prefix = QStringLiteral("image://gifthumb/");
    const QString string = url.toString();
    return string.startsWith(prefix) ? string.mid(prefix.size()) : QString();
}

QImage TestGifModel::solidImage(int width, int height, const QColor &color)
{
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(color);
    return image;
}

void TestGifModel::roleNamesMatchTheQmlContract()
{
    const QHash<int, QByteArray> roles = model.roleNames();

    QCOMPARE(roles.value(GifModel::FilePathRole), QByteArray("filePath"));
    QCOMPARE(roles.value(GifModel::FileNameRole), QByteArray("fileName"));
    QCOMPARE(roles.value(GifModel::FileUrlRole), QByteArray("fileUrl"));
    QCOMPARE(roles.value(GifModel::ThumbnailUrlRole), QByteArray("thumbnailUrl"));
    QCOMPARE(roles.value(GifModel::ThumbnailReadyRole), QByteArray("thumbnailReady"));
}

void TestGifModel::resetFilesPopulatesRows()
{
    model.resetFiles({QStringLiteral("/gifs/b.gif"), QStringLiteral("/gifs/a.gif")});

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0), GifModel::FilePathRole).toString(),
             QStringLiteral("/gifs/b.gif"));
    QCOMPARE(model.data(model.index(0), GifModel::FileNameRole).toString(),
             QStringLiteral("b.gif"));
    QCOMPARE(model.data(model.index(0), GifModel::FileUrlRole).toUrl(),
             QUrl::fromLocalFile(QStringLiteral("/gifs/b.gif")));
    QCOMPARE(model.data(model.index(1), GifModel::FileNameRole).toString(),
             QStringLiteral("a.gif"));
}

void TestGifModel::invalidIndexReturnsNothing()
{
    model.resetFiles({QStringLiteral("/gifs/a.gif")});

    QVERIFY(!model.data(QModelIndex(), GifModel::FilePathRole).isValid());
    QVERIFY(!model.data(model.index(5), GifModel::FilePathRole).isValid());
}

void TestGifModel::thumbnailStartsHidden()
{
    model.resetFiles({QStringLiteral("/gifs/a.gif")});

    QVERIFY(!model.data(model.index(0), GifModel::ThumbnailReadyRole).toBool());
    QVERIFY(!model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl().isValid());
}

void TestGifModel::setThumbnailPublishesImage()
{
    const QString path = QStringLiteral("/gifs/a.gif");
    model.resetFiles({path});

    QSignalSpy data_changed(&model, &QAbstractItemModel::dataChanged);
    model.setThumbnail(path, solidImage(4, 3, Qt::red));

    QCOMPARE(data_changed.count(), 1);
    QVERIFY(model.data(model.index(0), GifModel::ThumbnailReadyRole).toBool());

    const QUrl url = model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl();
    QVERIFY(url.toString().startsWith(QStringLiteral("image://gifthumb/")));

    QSize reported_size;
    const QImage image = provider.requestImage(imageIdFromUrl(url), &reported_size, QSize());
    QCOMPARE(image.size(), QSize(4, 3));
    QCOMPARE(reported_size, QSize(4, 3));
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
}

void TestGifModel::thumbnailRevisionChangesUrl()
{
    const QString path = QStringLiteral("/gifs/a.gif");
    model.resetFiles({path});

    model.setThumbnail(path, solidImage(4, 3, Qt::red));
    const QString first_id = imageIdFromUrl(
        model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl());

    model.setThumbnail(path, solidImage(4, 3, Qt::blue));
    const QString second_id = imageIdFromUrl(
        model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl());

    QVERIFY(!first_id.isEmpty());
    QVERIFY(!second_id.isEmpty());
    QVERIFY(first_id != second_id);

    // The old revision is released so the provider does not accumulate images.
    QVERIFY(provider.requestImage(first_id, nullptr, QSize()).isNull());
    QVERIFY(!provider.requestImage(second_id, nullptr, QSize()).isNull());
}

void TestGifModel::thumbnailIdChangesAcrossResets()
{
    const QString path = QStringLiteral("/gifs/a.gif");

    model.resetFiles({path});
    model.setThumbnail(path, solidImage(2, 2, Qt::green));
    const QString first_id = imageIdFromUrl(
        model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl());

    model.resetFiles({path});
    model.setThumbnail(path, solidImage(2, 2, Qt::green));
    const QString second_id = imageIdFromUrl(
        model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl());

    // QML caches thumbnails by URL, so a reload must not reuse the same id.
    QVERIFY(!first_id.isEmpty());
    QVERIFY(!second_id.isEmpty());
    QVERIFY(first_id != second_id);
}

void TestGifModel::removeFileShrinksModel()
{
    model.resetFiles({QStringLiteral("/gifs/a.gif"), QStringLiteral("/gifs/b.gif")});
    model.setThumbnail(QStringLiteral("/gifs/b.gif"), solidImage(2, 2, Qt::green));

    QVERIFY(model.removeFile(QStringLiteral("/gifs/a.gif")));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), GifModel::FilePathRole).toString(),
             QStringLiteral("/gifs/b.gif"));
    QVERIFY(model.data(model.index(0), GifModel::ThumbnailReadyRole).toBool());

    // The path index must have been rebuilt after the removal.
    QVERIFY(model.removeFile(QStringLiteral("/gifs/b.gif")));
    QCOMPARE(model.rowCount(), 0);
}

void TestGifModel::removeUnknownFileReturnsFalse()
{
    model.resetFiles({QStringLiteral("/gifs/a.gif")});

    QVERIFY(!model.removeFile(QStringLiteral("/gifs/missing.gif")));
    QCOMPARE(model.rowCount(), 1);
}

void TestGifModel::resetFilesClearsProvider()
{
    const QString first_path = QStringLiteral("/gifs/a.gif");
    model.resetFiles({first_path});
    model.setThumbnail(first_path, solidImage(2, 2, Qt::green));

    const QString old_id = imageIdFromUrl(
        model.data(model.index(0), GifModel::ThumbnailUrlRole).toUrl());
    QVERIFY(!old_id.isEmpty());

    model.resetFiles({QStringLiteral("/gifs/c.gif")});

    QVERIFY(provider.requestImage(old_id, nullptr, QSize()).isNull());
}

QTEST_GUILESS_MAIN(TestGifModel)

#include "tst_gif_model.moc"
