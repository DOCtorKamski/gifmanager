#include <QtTest>

#include "clipboard_service_desktop.h"

#include <QFile>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>

#include <memory>

namespace {

// Smallest well-known 1x1 GIF.
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

} // namespace

class TestClipboard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void localUrlCarriesFileNameW();
    void localUrlCarriesUriList();
    void localUrlCarriesImageWhenDecodable();
    void nonLocalUrlProducesNoPayload();

private:
    QTemporaryDir folder;
};

void TestClipboard::initTestCase()
{
    QVERIFY(folder.isValid());
    QVERIFY(writeFile(folder.filePath(QStringLiteral("sample.gif")), tinyGif()));
}

void TestClipboard::localUrlCarriesFileNameW()
{
    const QUrl url = QUrl::fromLocalFile(folder.filePath(QStringLiteral("sample.gif")));
    const std::unique_ptr<QMimeData> mime(createDesktopClipboardMimeData(url));

    QVERIFY(mime);
    QCOMPARE(mime->data("application/x-qt-windows-mime;value=\"FileNameW\""),
             QByteArrayLiteral("sample.gif"));
}

void TestClipboard::localUrlCarriesUriList()
{
    const QUrl url = QUrl::fromLocalFile(folder.filePath(QStringLiteral("sample.gif")));
    const std::unique_ptr<QMimeData> mime(createDesktopClipboardMimeData(url));

    QVERIFY(mime);
    QCOMPARE(mime->urls(), QList<QUrl>() << url);
}

void TestClipboard::localUrlCarriesImageWhenDecodable()
{
    const QString path = folder.filePath(QStringLiteral("sample.gif"));
    if (!QImageReader(path).canRead())
        QSKIP("No image plugin can decode GIF on this system");

    const std::unique_ptr<QMimeData> mime(
        createDesktopClipboardMimeData(QUrl::fromLocalFile(path)));

    QVERIFY(mime);
    QVERIFY(mime->hasImage());
}

void TestClipboard::nonLocalUrlProducesNoPayload()
{
    QVERIFY(!createDesktopClipboardMimeData(
        QUrl(QStringLiteral("content://provider/document/a.gif"))));
}

int main(int argc, char *argv[])
{
    // The payload embeds a QPixmap, which needs a QGuiApplication. The offscreen
    // platform keeps the test headless; an explicit -platform still wins.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));

    QGuiApplication app(argc, argv);
    TestClipboard test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "tst_clipboard.moc"
