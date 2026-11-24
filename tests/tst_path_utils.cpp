#include <QtTest>

#include "path_utils.h"

#include <QDir>
#include <QUrl>

class TestPathUtils : public QObject
{
    Q_OBJECT

private slots:
    void emptyInputProducesNoUrl();
    void keepsFileUrls();
    void keepsContentUrls();
    void plainPathBecomesLocalFileUrl();
    void toLocalPathReturnsPathForFileUrls();
    void toLocalPathReturnsUrlStringForOtherSchemes();
    void baseNameDropsExtension();
};

void TestPathUtils::emptyInputProducesNoUrl()
{
    QVERIFY(PathUtils::instance().fromUserInput(QString()).isEmpty());
}

void TestPathUtils::keepsFileUrls()
{
    const QUrl url(QStringLiteral("file:///tmp/some%20folder/a.gif"));
    QCOMPARE(PathUtils::instance().fromUserInput(url.toString()), url);
}

void TestPathUtils::keepsContentUrls()
{
    const QUrl url(QStringLiteral("content://com.android.externalstorage.documents"
                                  "/tree/primary%3AGifs/document/primary%3AGifs%2Fa.gif"));
    QCOMPARE(PathUtils::instance().fromUserInput(url.toString()), url);
}

void TestPathUtils::plainPathBecomesLocalFileUrl()
{
    const QString path = QDir::tempPath() + QStringLiteral("/gifmanager/gifs");
    const QUrl url = PathUtils::instance().fromUserInput(path);
    QVERIFY(url.isLocalFile());
    QCOMPARE(url, QUrl::fromLocalFile(path));
}

void TestPathUtils::toLocalPathReturnsPathForFileUrls()
{
    const QUrl url = QUrl::fromLocalFile(QDir::tempPath()
                                         + QStringLiteral("/gifmanager/a.gif"));
    QCOMPARE(PathUtils::instance().toLocalPath(url), url.toLocalFile());
}

void TestPathUtils::toLocalPathReturnsUrlStringForOtherSchemes()
{
    const QUrl url(QStringLiteral("content://provider/document/a.gif"));
    QCOMPARE(PathUtils::instance().toLocalPath(url), url.toString());
}

void TestPathUtils::baseNameDropsExtension()
{
    QCOMPARE(PathUtils::instance().baseName(QStringLiteral("photo.gif")),
             QStringLiteral("photo"));
    // QFileInfo::baseName() stops at the first dot, not the last one, so a
    // dotted name prefills the rename dialog with "photo" instead of
    // "photo.final". This locks the current behaviour.
    QCOMPARE(PathUtils::instance().baseName(QStringLiteral("photo.final.gif")),
             QStringLiteral("photo"));
    QCOMPARE(PathUtils::instance().baseName(QStringLiteral("photo")),
             QStringLiteral("photo"));
}

QTEST_GUILESS_MAIN(TestPathUtils)

#include "tst_path_utils.moc"
