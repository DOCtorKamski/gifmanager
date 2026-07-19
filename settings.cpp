#include "settings.h"

SettingsManager::SettingsManager()
    : settings("softBy_DOCtorKamski", "gifmanager")
{
}

SettingsManager& SettingsManager::instance() {
    static SettingsManager inst;
    return inst;
}

QSize SettingsManager::getThumbnailSize() const {
    return settings.value(KEY_THUMB_SIZE, DEFAULT_THUMB_SIZE).toSize();
}

void SettingsManager::setThumbnailSize(const QSize &size) {
    settings.setValue(KEY_THUMB_SIZE, size);
    settings.sync();
}

int SettingsManager::getColumns() const {
    return settings.value(KEY_COLUMNS, DEFAULT_COLUMNS).toInt();
}

void SettingsManager::setColumns(int columns) {
    settings.setValue(KEY_COLUMNS, columns);
    settings.sync();
}

QString SettingsManager::getThumbnailStyle() const {
    return settings.value(KEY_THUMB_STYLE, defaultThumbnailStyle()).toString();
}

void SettingsManager::setThumbnailStyle(const QString &style) {
    settings.setValue(KEY_THUMB_STYLE, style);
    settings.sync();
}

QString SettingsManager::getDefaultFolder() const {
    return settings.value(KEY_DEFAULT_FOLDER, "").toString();
}

void SettingsManager::setDefaultFolder(const QString &path) {
    settings.setValue(KEY_DEFAULT_FOLDER, path);
    settings.sync();
}

int SettingsManager::getSearchDebounceMs() const {
    return settings.value(KEY_SEARCH_DEBOUNCE, DEFAULT_SEARCH_DEBOUNCE).toInt();
}

void SettingsManager::setSearchDebounceMs(int ms) {
    settings.setValue(KEY_SEARCH_DEBOUNCE, ms);
    settings.sync();
}

int SettingsManager::getVisibilityDebounceMs() const {
    return settings.value(KEY_VISIBILITY_DEBOUNCE, DEFAULT_VISIBILITY_DEBOUNCE).toInt();
}

void SettingsManager::setVisibilityDebounceMs(int ms) {
    settings.setValue(KEY_VISIBILITY_DEBOUNCE, ms);
    settings.sync();
}

QSize SettingsManager::getViewerSize() const {
    return settings.value(KEY_VIEWER_SIZE, DEFAULT_VIEWER_SIZE).toSize();
}

void SettingsManager::setViewerSize(const QSize &size) {
    settings.setValue(KEY_VIEWER_SIZE, size);
    settings.sync();
}

QString SettingsManager::getViewerBgStyle() const {
    return settings.value(KEY_VIEWER_BG_STYLE, defaultViewerBgStyle()).toString();
}

void SettingsManager::setViewerBgStyle(const QString &style) {
    settings.setValue(KEY_VIEWER_BG_STYLE, style);
    settings.sync();
}

QString SettingsManager::getViewerLabelStyle() const {
    return settings.value(KEY_VIEWER_LABEL_STYLE, defaultViewerLabelStyle()).toString();
}

void SettingsManager::setViewerLabelStyle(const QString &style) {
    settings.setValue(KEY_VIEWER_LABEL_STYLE, style);
    settings.sync();
}

int SettingsManager::getGridHorizontalSpacing() const {
    return settings.value(KEY_GRID_HSPACING, DEFAULT_GRID_HSPACING).toInt();
}

void SettingsManager::setGridHorizontalSpacing(int spacing) {
    settings.setValue(KEY_GRID_HSPACING, spacing);
    settings.sync();
}

int SettingsManager::getGridVerticalSpacing() const {
    return settings.value(KEY_GRID_VSPACING, DEFAULT_GRID_VSPACING).toInt();
}

void SettingsManager::setGridVerticalSpacing(int spacing) {
    settings.setValue(KEY_GRID_VSPACING, spacing);
    settings.sync();
}

QByteArray SettingsManager::getWindowGeometry() const {
    return settings.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void SettingsManager::setWindowGeometry(const QByteArray &geometry) {
    settings.setValue(KEY_WINDOW_GEOMETRY, geometry);
    settings.sync();
}

void SettingsManager::resetToDefaults() {
    QByteArray geom = settings.value(KEY_WINDOW_GEOMETRY).toByteArray();
    settings.clear();
    if (!geom.isEmpty())
        settings.setValue(KEY_WINDOW_GEOMETRY, geom);
    settings.sync();
}
