#ifndef SETTINGS_H
#define SETTINGS_H

#include <QByteArray>
#include <QObject>
#include <QRect>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QVariantMap>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSize thumbnailSize READ getThumbnailSize WRITE setThumbnailSize
               NOTIFY thumbnailSizeChanged)
    Q_PROPERTY(int thumbnailWidth READ thumbnailWidth WRITE setThumbnailWidth
               NOTIFY thumbnailSizeChanged)
    Q_PROPERTY(int thumbnailHeight READ thumbnailHeight WRITE setThumbnailHeight
               NOTIFY thumbnailSizeChanged)
    Q_PROPERTY(int columns READ getColumns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(QString thumbnailStyle READ getThumbnailStyle WRITE setThumbnailStyle
               NOTIFY thumbnailStyleChanged)
    Q_PROPERTY(QString defaultFolder READ getDefaultFolder WRITE setDefaultFolder
               NOTIFY defaultFolderChanged)
    Q_PROPERTY(int searchDebounceMs READ getSearchDebounceMs WRITE setSearchDebounceMs
               NOTIFY searchDebounceMsChanged)
    Q_PROPERTY(int visibilityDebounceMs READ getVisibilityDebounceMs
               WRITE setVisibilityDebounceMs NOTIFY visibilityDebounceMsChanged)
    Q_PROPERTY(QSize viewerSize READ getViewerSize WRITE setViewerSize
               NOTIFY viewerSizeChanged)
    Q_PROPERTY(int viewerWidth READ viewerWidth WRITE setViewerWidth
               NOTIFY viewerSizeChanged)
    Q_PROPERTY(int viewerHeight READ viewerHeight WRITE setViewerHeight
               NOTIFY viewerSizeChanged)
    Q_PROPERTY(QString viewerBgStyle READ getViewerBgStyle WRITE setViewerBgStyle
               NOTIFY viewerBgStyleChanged)
    Q_PROPERTY(QString viewerLabelStyle READ getViewerLabelStyle WRITE setViewerLabelStyle
               NOTIFY viewerLabelStyleChanged)
    Q_PROPERTY(int gridHorizontalSpacing READ getGridHorizontalSpacing
               WRITE setGridHorizontalSpacing NOTIFY gridHorizontalSpacingChanged)
    Q_PROPERTY(int gridVerticalSpacing READ getGridVerticalSpacing
               WRITE setGridVerticalSpacing NOTIFY gridVerticalSpacingChanged)
    Q_PROPERTY(int gridScrollSpeed READ getGridScrollSpeed WRITE setGridScrollSpeed
               NOTIFY gridScrollSpeedChanged)
    Q_PROPERTY(QRect windowGeometry READ windowGeometry WRITE setWindowGeometry
               NOTIFY windowGeometryChanged)
    Q_PROPERTY(bool hasWindowGeometry READ hasWindowGeometry NOTIFY windowGeometryChanged)

public:
    static SettingsManager& instance();

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSize getThumbnailSize() const;
    void setThumbnailSize(const QSize &size);
    int thumbnailWidth() const { return getThumbnailSize().width(); }
    int thumbnailHeight() const { return getThumbnailSize().height(); }
    void setThumbnailWidth(int width) {
        setThumbnailSize(QSize(width, getThumbnailSize().height()));
    }
    void setThumbnailHeight(int height) {
        setThumbnailSize(QSize(getThumbnailSize().width(), height));
    }

    int getColumns() const;
    void setColumns(int columns);

    QString getThumbnailStyle() const;
    void setThumbnailStyle(const QString &style);

    QString getDefaultFolder() const;
    void setDefaultFolder(const QString &path);

    int getSearchDebounceMs() const;
    void setSearchDebounceMs(int ms);

    int getVisibilityDebounceMs() const;
    void setVisibilityDebounceMs(int ms);

    QSize getViewerSize() const;
    void setViewerSize(const QSize &size);
    int viewerWidth() const { return getViewerSize().width(); }
    int viewerHeight() const { return getViewerSize().height(); }
    void setViewerWidth(int width) {
        setViewerSize(QSize(width, getViewerSize().height()));
    }
    void setViewerHeight(int height) {
        setViewerSize(QSize(getViewerSize().width(), height));
    }

    QString getViewerBgStyle() const;
    void setViewerBgStyle(const QString &style);

    QString getViewerLabelStyle() const;
    void setViewerLabelStyle(const QString &style);

    int getGridHorizontalSpacing() const;
    void setGridHorizontalSpacing(int spacing);

    int getGridVerticalSpacing() const;
    void setGridVerticalSpacing(int spacing);

    int getGridScrollSpeed() const;
    void setGridScrollSpeed(int speed);

    QRect windowGeometry() const;
    void setWindowGeometry(const QRect &geometry);
    bool hasWindowGeometry() const;

    Q_INVOKABLE QVariantMap defaultValues() const;
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height);

signals:
    void thumbnailSizeChanged();
    void columnsChanged();
    void thumbnailStyleChanged();
    void defaultFolderChanged();
    void searchDebounceMsChanged();
    void visibilityDebounceMsChanged();
    void viewerSizeChanged();
    void viewerBgStyleChanged();
    void viewerLabelStyleChanged();
    void gridHorizontalSpacingChanged();
    void gridVerticalSpacingChanged();
    void gridScrollSpeedChanged();
    void windowGeometryChanged();

private:
    SettingsManager();

    QSettings settings;

    static constexpr QSize DEFAULT_THUMB_SIZE = QSize(200, 160);
    static constexpr int DEFAULT_COLUMNS = 3;
    static constexpr int DEFAULT_SEARCH_DEBOUNCE = 200;
    static constexpr int DEFAULT_VISIBILITY_DEBOUNCE = 50;
    static constexpr QSize DEFAULT_VIEWER_SIZE = QSize(800, 600);
    static constexpr int DEFAULT_GRID_HSPACING = 6;
    static constexpr int DEFAULT_GRID_VSPACING = 4;
    static constexpr int DEFAULT_GRID_SCROLL_SPEED = 72;

    static inline QString defaultThumbnailStyle() {
        return "background: #999; border: 1px solid #444;";
    }
    static inline QString defaultViewerBgStyle() {
        return "background: #000;";
    }
    static inline QString defaultViewerLabelStyle() {
        return "background: #333; color: #fff; padding: 4px;";
    }

    static constexpr const char* KEY_THUMB_SIZE = "thumbnail/size";
    static constexpr const char* KEY_COLUMNS = "layout/columns";
    static constexpr const char* KEY_THUMB_STYLE = "thumbnail/style";
    static constexpr const char* KEY_DEFAULT_FOLDER = "folder/default";
    static constexpr const char* KEY_SEARCH_DEBOUNCE = "timers/searchDebounceMs";
    static constexpr const char* KEY_VISIBILITY_DEBOUNCE = "timers/visibilityDebounceMs";
    static constexpr const char* KEY_VIEWER_SIZE = "viewer/size";
    static constexpr const char* KEY_VIEWER_BG_STYLE = "viewer/bgStyle";
    static constexpr const char* KEY_VIEWER_LABEL_STYLE = "viewer/labelStyle";
    static constexpr const char* KEY_GRID_HSPACING = "layout/gridHorizontalSpacing";
    static constexpr const char* KEY_GRID_VSPACING = "layout/gridVerticalSpacing";
    static constexpr const char* KEY_GRID_SCROLL_SPEED = "layout/gridScrollSpeed";
    static constexpr const char* KEY_WINDOW_RECT = "window/rect";
};

#endif // SETTINGS_H
