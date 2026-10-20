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
    emit thumbnailSizeChanged();
}

int SettingsManager::getColumns() const {
    return settings.value(KEY_COLUMNS, DEFAULT_COLUMNS).toInt();
}

void SettingsManager::setColumns(int columns) {
    settings.setValue(KEY_COLUMNS, columns);
    settings.sync();
    emit columnsChanged();
}

QString SettingsManager::getThumbnailStyle() const {
    return settings.value(KEY_THUMB_STYLE, defaultThumbnailStyle()).toString();
}

void SettingsManager::setThumbnailStyle(const QString &style) {
    settings.setValue(KEY_THUMB_STYLE, style);
    settings.sync();
    emit thumbnailStyleChanged();
}

QString SettingsManager::getDefaultFolder() const {
    return settings.value(KEY_DEFAULT_FOLDER, "").toString();
}

void SettingsManager::setDefaultFolder(const QString &path) {
    settings.setValue(KEY_DEFAULT_FOLDER, path);
    settings.sync();
    emit defaultFolderChanged();
}

int SettingsManager::getSearchDebounceMs() const {
    return settings.value(KEY_SEARCH_DEBOUNCE, DEFAULT_SEARCH_DEBOUNCE).toInt();
}

void SettingsManager::setSearchDebounceMs(int ms) {
    settings.setValue(KEY_SEARCH_DEBOUNCE, ms);
    settings.sync();
    emit searchDebounceMsChanged();
}

int SettingsManager::getVisibilityDebounceMs() const {
    return settings.value(KEY_VISIBILITY_DEBOUNCE, DEFAULT_VISIBILITY_DEBOUNCE).toInt();
}

void SettingsManager::setVisibilityDebounceMs(int ms) {
    settings.setValue(KEY_VISIBILITY_DEBOUNCE, ms);
    settings.sync();
    emit visibilityDebounceMsChanged();
}

QSize SettingsManager::getViewerSize() const {
    return settings.value(KEY_VIEWER_SIZE, DEFAULT_VIEWER_SIZE).toSize();
}

void SettingsManager::setViewerSize(const QSize &size) {
    settings.setValue(KEY_VIEWER_SIZE, size);
    settings.sync();
    emit viewerSizeChanged();
}

QString SettingsManager::getViewerBgStyle() const {
    return settings.value(KEY_VIEWER_BG_STYLE, defaultViewerBgStyle()).toString();
}

void SettingsManager::setViewerBgStyle(const QString &style) {
    settings.setValue(KEY_VIEWER_BG_STYLE, style);
    settings.sync();
    emit viewerBgStyleChanged();
}

QString SettingsManager::getViewerLabelStyle() const {
    return settings.value(KEY_VIEWER_LABEL_STYLE, defaultViewerLabelStyle()).toString();
}

void SettingsManager::setViewerLabelStyle(const QString &style) {
    settings.setValue(KEY_VIEWER_LABEL_STYLE, style);
    settings.sync();
    emit viewerLabelStyleChanged();
}

int SettingsManager::getGridHorizontalSpacing() const {
    return settings.value(KEY_GRID_HSPACING, DEFAULT_GRID_HSPACING).toInt();
}

void SettingsManager::setGridHorizontalSpacing(int spacing) {
    settings.setValue(KEY_GRID_HSPACING, spacing);
    settings.sync();
    emit gridHorizontalSpacingChanged();
}

int SettingsManager::getGridVerticalSpacing() const {
    return settings.value(KEY_GRID_VSPACING, DEFAULT_GRID_VSPACING).toInt();
}

void SettingsManager::setGridVerticalSpacing(int spacing) {
    settings.setValue(KEY_GRID_VSPACING, spacing);
    settings.sync();
    emit gridVerticalSpacingChanged();
}

int SettingsManager::getGridScrollSpeed() const {
    return settings.value(KEY_GRID_SCROLL_SPEED, DEFAULT_GRID_SCROLL_SPEED).toInt();
}

void SettingsManager::setGridScrollSpeed(int speed) {
    settings.setValue(KEY_GRID_SCROLL_SPEED, speed);
    settings.sync();
    emit gridScrollSpeedChanged();
}

QRect SettingsManager::windowGeometry() const {
    return settings.value(KEY_WINDOW_RECT, QRect()).toRect();
}

void SettingsManager::setWindowGeometry(const QRect &geometry) {
    settings.setValue(KEY_WINDOW_RECT, geometry);
    settings.sync();
    emit windowGeometryChanged();
}

bool SettingsManager::hasWindowGeometry() const {
    return settings.contains(KEY_WINDOW_RECT);
}

QVariantMap SettingsManager::defaultValues() const {
    QVariantMap values;
    values.insert(QStringLiteral("thumbnailWidth"), DEFAULT_THUMB_SIZE.width());
    values.insert(QStringLiteral("thumbnailHeight"), DEFAULT_THUMB_SIZE.height());
    values.insert(QStringLiteral("columns"), DEFAULT_COLUMNS);
    values.insert(QStringLiteral("defaultFolder"), QString());
    values.insert(QStringLiteral("searchDebounceMs"), DEFAULT_SEARCH_DEBOUNCE);
    values.insert(QStringLiteral("visibilityDebounceMs"), DEFAULT_VISIBILITY_DEBOUNCE);
    values.insert(QStringLiteral("viewerWidth"), DEFAULT_VIEWER_SIZE.width());
    values.insert(QStringLiteral("viewerHeight"), DEFAULT_VIEWER_SIZE.height());
    values.insert(QStringLiteral("gridHorizontalSpacing"), DEFAULT_GRID_HSPACING);
    values.insert(QStringLiteral("gridVerticalSpacing"), DEFAULT_GRID_VSPACING);
    values.insert(QStringLiteral("gridScrollSpeed"), DEFAULT_GRID_SCROLL_SPEED);
    return values;
}

void SettingsManager::resetToDefaults() {
    const bool had_window_geometry = hasWindowGeometry();
    const QRect saved_geometry = windowGeometry();

    settings.clear();
    if (had_window_geometry)
        settings.setValue(KEY_WINDOW_RECT, saved_geometry);
    settings.sync();

    emit thumbnailSizeChanged();
    emit columnsChanged();
    emit thumbnailStyleChanged();
    emit defaultFolderChanged();
    emit searchDebounceMsChanged();
    emit visibilityDebounceMsChanged();
    emit viewerSizeChanged();
    emit viewerBgStyleChanged();
    emit viewerLabelStyleChanged();
    emit gridHorizontalSpacingChanged();
    emit gridVerticalSpacingChanged();
    emit gridScrollSpeedChanged();
    emit windowGeometryChanged();
}

void SettingsManager::saveWindowGeometry(int x, int y, int width, int height) {
    setWindowGeometry(QRect(x, y, width, height));
}
