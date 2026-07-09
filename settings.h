#ifndef SETTINGS_H
#define SETTINGS_H

#include <QByteArray>
#include <QSettings>
#include <QSize>
#include <QString>

class SettingsManager {
public:
    static SettingsManager& instance();

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSize getThumbnailSize() const;
    void setThumbnailSize(const QSize &size);

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

    QString getViewerBgStyle() const;
    void setViewerBgStyle(const QString &style);

    QString getViewerLabelStyle() const;
    void setViewerLabelStyle(const QString &style);

    int getGridHorizontalSpacing() const;
    void setGridHorizontalSpacing(int spacing);

    int getGridVerticalSpacing() const;
    void setGridVerticalSpacing(int spacing);

    QByteArray getWindowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

    void resetToDefaults();

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
    static constexpr const char* KEY_WINDOW_GEOMETRY = "window/geometry";
};

#endif // SETTINGS_H
