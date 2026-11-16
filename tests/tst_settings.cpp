#include <QtTest>

#include "settings.h"

#include <QRect>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrl>

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void defaultsAreReportedWhenStorageIsEmpty();
    void valuesRoundTripThroughQSettings();
    void legacyStringDefaultFolderIsMigrated();
    void defaultValuesCoverEverySetting();
    void resetToDefaultsRestoresValues();
    void resetToDefaultsKeepsWindowGeometry();
    void widthAndHeightSettersKeepTheOtherDimension();
    void signalsAreEmittedOnChange();
    void batchUpdateReadsBackValues();

private:
    QTemporaryDir settings_dir;
};

void TestSettings::initTestCase()
{
    QVERIFY(settings_dir.isValid());

    // Isolate from the real user configuration: both the file format and the
    // directory have to be redirected before SettingsManager constructs its
    // QSettings instance.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir.path());
}

void TestSettings::init()
{
    QSettings probe(AppIdentity::ORGANIZATION, AppIdentity::APPLICATION);
    probe.clear();
    probe.sync();
}

void TestSettings::defaultsAreReportedWhenStorageIsEmpty()
{
    SettingsManager settings;

    QCOMPARE(settings.getThumbnailSize(), QSize(210, 170));
    QCOMPARE(settings.getColumns(), 3);
    QVERIFY(settings.getDefaultFolderUrl().isEmpty());
    QCOMPARE(settings.getSearchDebounceMs(), 200);
    QCOMPARE(settings.getVisibilityDebounceMs(), 50);
    QCOMPARE(settings.getViewerSize(), QSize(800, 600));
    QCOMPARE(settings.getGridHorizontalSpacing(), 6);
    QCOMPARE(settings.getGridVerticalSpacing(), 4);
    QCOMPARE(settings.getGridScrollSpeed(), 72);
    QVERIFY(!settings.hasWindowGeometry());
    QCOMPARE(settings.windowGeometry(), QRect());
}

void TestSettings::valuesRoundTripThroughQSettings()
{
    SettingsManager settings;

    settings.setThumbnailSize(QSize(120, 90));
    settings.setColumns(4);
    settings.setDefaultFolderUrl(QUrl::fromLocalFile(QStringLiteral("/gifs")));
    settings.setSearchDebounceMs(111);
    settings.setVisibilityDebounceMs(22);
    settings.setViewerSize(QSize(640, 480));
    settings.setGridHorizontalSpacing(1);
    settings.setGridVerticalSpacing(2);
    settings.setGridScrollSpeed(36);
    settings.setWindowGeometry(QRect(10, 20, 300, 400));

    // The INI keys are part of the on-disk contract; changing them silently
    // would drop the user's existing settings.
    QSettings probe(AppIdentity::ORGANIZATION, AppIdentity::APPLICATION);
    QCOMPARE(probe.value(QStringLiteral("thumbnail/size")).toSize(), QSize(120, 90));
    QCOMPARE(probe.value(QStringLiteral("layout/columns")).toInt(), 4);
    QCOMPARE(probe.value(QStringLiteral("folder/default")).toString(),
             QUrl::fromLocalFile(QStringLiteral("/gifs")).toString());
    QCOMPARE(probe.value(QStringLiteral("timers/searchDebounceMs")).toInt(), 111);
    QCOMPARE(probe.value(QStringLiteral("timers/visibilityDebounceMs")).toInt(), 22);
    QCOMPARE(probe.value(QStringLiteral("viewer/size")).toSize(), QSize(640, 480));
    QCOMPARE(probe.value(QStringLiteral("layout/gridHorizontalSpacing")).toInt(), 1);
    QCOMPARE(probe.value(QStringLiteral("layout/gridVerticalSpacing")).toInt(), 2);
    QCOMPARE(probe.value(QStringLiteral("layout/gridScrollSpeed")).toInt(), 36);
    QCOMPARE(probe.value(QStringLiteral("window/rect")).toRect(), QRect(10, 20, 300, 400));
}

void TestSettings::legacyStringDefaultFolderIsMigrated()
{
    // Older builds stored folder/default as a plain path. It must still be
    // picked up after the switch to QUrl.
    QSettings probe(AppIdentity::ORGANIZATION, AppIdentity::APPLICATION);
    probe.setValue(QStringLiteral("folder/default"), QStringLiteral("/legacy/gifs"));
    probe.sync();

    SettingsManager settings;
    QCOMPARE(settings.getDefaultFolderUrl(),
             QUrl::fromLocalFile(QStringLiteral("/legacy/gifs")));
}

void TestSettings::defaultValuesCoverEverySetting()
{
    SettingsManager settings;
    const QVariantMap values = settings.defaultValues();

    const QStringList expected = {
        QStringLiteral("thumbnailSize"),
        QStringLiteral("thumbnailWidth"),
        QStringLiteral("thumbnailHeight"),
        QStringLiteral("columns"),
        QStringLiteral("defaultFolderUrl"),
        QStringLiteral("searchDebounceMs"),
        QStringLiteral("visibilityDebounceMs"),
        QStringLiteral("viewerSize"),
        QStringLiteral("viewerWidth"),
        QStringLiteral("viewerHeight"),
        QStringLiteral("gridHorizontalSpacing"),
        QStringLiteral("gridVerticalSpacing"),
        QStringLiteral("gridScrollSpeed"),
    };

    for (const QString &key : expected)
        QVERIFY2(values.contains(key), qPrintable(key));

    QCOMPARE(values.value(QStringLiteral("thumbnailWidth")).toInt(), 210);
    QCOMPARE(values.value(QStringLiteral("thumbnailHeight")).toInt(), 170);
    QCOMPARE(values.value(QStringLiteral("viewerWidth")).toInt(), 800);
    QCOMPARE(values.value(QStringLiteral("viewerHeight")).toInt(), 600);
}

void TestSettings::resetToDefaultsRestoresValues()
{
    SettingsManager settings;
    settings.setColumns(7);
    settings.setThumbnailSize(QSize(400, 300));
    settings.setDefaultFolderUrl(QUrl::fromLocalFile(QStringLiteral("/elsewhere")));

    QSignalSpy columns_changed(&settings, &SettingsManager::columnsChanged);
    settings.resetToDefaults();

    QCOMPARE(columns_changed.count(), 1);
    QCOMPARE(settings.getColumns(), 3);
    QCOMPARE(settings.getThumbnailSize(), QSize(210, 170));
    QVERIFY(settings.getDefaultFolderUrl().isEmpty());
}

void TestSettings::resetToDefaultsKeepsWindowGeometry()
{
    SettingsManager settings;
    settings.setWindowGeometry(QRect(1, 2, 3, 4));

    settings.resetToDefaults();

    QVERIFY(settings.hasWindowGeometry());
    QCOMPARE(settings.windowGeometry(), QRect(1, 2, 3, 4));
}

void TestSettings::widthAndHeightSettersKeepTheOtherDimension()
{
    SettingsManager settings;

    settings.setThumbnailSize(QSize(200, 160));
    settings.setThumbnailWidth(64);
    QCOMPARE(settings.getThumbnailSize(), QSize(64, 160));
    settings.setThumbnailHeight(48);
    QCOMPARE(settings.getThumbnailSize(), QSize(64, 48));

    settings.setViewerSize(QSize(800, 600));
    settings.setViewerWidth(500);
    QCOMPARE(settings.getViewerSize(), QSize(500, 600));
    settings.setViewerHeight(320);
    QCOMPARE(settings.getViewerSize(), QSize(500, 320));
}

void TestSettings::signalsAreEmittedOnChange()
{
    SettingsManager settings;

    QSignalSpy columns_changed(&settings, &SettingsManager::columnsChanged);
    QSignalSpy thumbnail_size_changed(&settings, &SettingsManager::thumbnailSizeChanged);
    QSignalSpy folder_changed(&settings, &SettingsManager::defaultFolderChanged);

    settings.setColumns(5);
    settings.setThumbnailWidth(210);
    settings.setDefaultFolderUrl(QUrl::fromLocalFile(QStringLiteral("/gifs")));

    QCOMPARE(columns_changed.count(), 1);
    QCOMPARE(thumbnail_size_changed.count(), 1);
    QCOMPARE(folder_changed.count(), 1);
}

void TestSettings::batchUpdateReadsBackValues()
{
    SettingsManager settings;

    settings.beginUpdate();
    settings.setThumbnailSize(QSize(100, 80));
    settings.setColumns(2);
    settings.endUpdate();

    QCOMPARE(settings.getThumbnailSize(), QSize(100, 80));
    QCOMPARE(settings.getColumns(), 2);

    QSettings probe(AppIdentity::ORGANIZATION, AppIdentity::APPLICATION);
    QCOMPARE(probe.value(QStringLiteral("thumbnail/size")).toSize(), QSize(100, 80));
    QCOMPARE(probe.value(QStringLiteral("layout/columns")).toInt(), 2);
}

QTEST_GUILESS_MAIN(TestSettings)

#include "tst_settings.moc"
