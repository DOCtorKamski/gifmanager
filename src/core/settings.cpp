#include "settings.h"

#include <QMetaType>

SettingsManager::SettingsManager()
    : settings(AppIdentity::ORGANIZATION, AppIdentity::APPLICATION)
{
}

SettingsManager& SettingsManager::instance() {
    static SettingsManager inst;
    return inst;
}

const QList<SettingsManager::SettingDef> &SettingsManager::settingDefs()
{
    // Single source of truth for persisted settings: storage key, QML name,
    // default value, and the notification emitted when defaults are restored.
    static const QList<SettingDef> defs = {
        {KEY_THUMB_SIZE, "thumbnailSize", DEFAULT_THUMB_SIZE,
         &SettingsManager::thumbnailSizeChanged},
        {KEY_COLUMNS, "columns", DEFAULT_COLUMNS,
         &SettingsManager::columnsChanged},
        {KEY_DEFAULT_FOLDER, "defaultFolderUrl", QUrl(),
         &SettingsManager::defaultFolderChanged},
        {KEY_SEARCH_DEBOUNCE, "searchDebounceMs", DEFAULT_SEARCH_DEBOUNCE,
         &SettingsManager::searchDebounceMsChanged},
        {KEY_VISIBILITY_DEBOUNCE, "visibilityDebounceMs", DEFAULT_VISIBILITY_DEBOUNCE,
         &SettingsManager::visibilityDebounceMsChanged},
        {KEY_VIEWER_SIZE, "viewerSize", DEFAULT_VIEWER_SIZE,
         &SettingsManager::viewerSizeChanged},
        {KEY_GRID_HSPACING, "gridHorizontalSpacing", DEFAULT_GRID_HSPACING,
         &SettingsManager::gridHorizontalSpacingChanged},
        {KEY_GRID_VSPACING, "gridVerticalSpacing", DEFAULT_GRID_VSPACING,
         &SettingsManager::gridVerticalSpacingChanged},
        {KEY_GRID_SCROLL_SPEED, "gridScrollSpeed", DEFAULT_GRID_SCROLL_SPEED,
         &SettingsManager::gridScrollSpeedChanged},
    };
    return defs;
}

void SettingsManager::writeSetting(const char *key, const QVariant &value,
                                   void (SettingsManager::*notify)())
{
    settings.setValue(key, value);
    if (update_depth == 0)
        settings.sync();
    (this->*notify)();
}

QSize SettingsManager::getThumbnailSize() const {
    return settings.value(KEY_THUMB_SIZE, DEFAULT_THUMB_SIZE).toSize();
}

void SettingsManager::setThumbnailSize(const QSize &size) {
    writeSetting(KEY_THUMB_SIZE, size, &SettingsManager::thumbnailSizeChanged);
}

int SettingsManager::getColumns() const {
    return settings.value(KEY_COLUMNS, DEFAULT_COLUMNS).toInt();
}

void SettingsManager::setColumns(int columns) {
    writeSetting(KEY_COLUMNS, columns, &SettingsManager::columnsChanged);
}

QUrl SettingsManager::getDefaultFolderUrl() const {
    const QVariant raw = settings.value(KEY_DEFAULT_FOLDER);
    if (raw.metaType().id() == QMetaType::QUrl)
        return raw.toUrl();

    // Migrate the legacy desktop value, which was stored as a plain path.
    const QString text = raw.toString();
    if (text.isEmpty())
        return {};

    const QUrl url(text);
    if (url.scheme() == QLatin1String("file")
        || url.scheme() == QLatin1String("content")) {
        return url;
    }
    return QUrl::fromLocalFile(text);
}

void SettingsManager::setDefaultFolderUrl(const QUrl &url) {
    // Store a plain string: QSettings' INI backend is not guaranteed to
    // preserve the QUrl type, and older builds wrote a plain path here.
    writeSetting(KEY_DEFAULT_FOLDER, url.toString(),
                 &SettingsManager::defaultFolderChanged);
}

int SettingsManager::getSearchDebounceMs() const {
    return settings.value(KEY_SEARCH_DEBOUNCE, DEFAULT_SEARCH_DEBOUNCE).toInt();
}

void SettingsManager::setSearchDebounceMs(int ms) {
    writeSetting(KEY_SEARCH_DEBOUNCE, ms, &SettingsManager::searchDebounceMsChanged);
}

int SettingsManager::getVisibilityDebounceMs() const {
    return settings.value(KEY_VISIBILITY_DEBOUNCE, DEFAULT_VISIBILITY_DEBOUNCE).toInt();
}

void SettingsManager::setVisibilityDebounceMs(int ms) {
    writeSetting(KEY_VISIBILITY_DEBOUNCE, ms,
                 &SettingsManager::visibilityDebounceMsChanged);
}

QSize SettingsManager::getViewerSize() const {
    return settings.value(KEY_VIEWER_SIZE, DEFAULT_VIEWER_SIZE).toSize();
}

void SettingsManager::setViewerSize(const QSize &size) {
    writeSetting(KEY_VIEWER_SIZE, size, &SettingsManager::viewerSizeChanged);
}

int SettingsManager::getGridHorizontalSpacing() const {
    return settings.value(KEY_GRID_HSPACING, DEFAULT_GRID_HSPACING).toInt();
}

void SettingsManager::setGridHorizontalSpacing(int spacing) {
    writeSetting(KEY_GRID_HSPACING, spacing,
                 &SettingsManager::gridHorizontalSpacingChanged);
}

int SettingsManager::getGridVerticalSpacing() const {
    return settings.value(KEY_GRID_VSPACING, DEFAULT_GRID_VSPACING).toInt();
}

void SettingsManager::setGridVerticalSpacing(int spacing) {
    writeSetting(KEY_GRID_VSPACING, spacing,
                 &SettingsManager::gridVerticalSpacingChanged);
}

int SettingsManager::getGridScrollSpeed() const {
    return settings.value(KEY_GRID_SCROLL_SPEED, DEFAULT_GRID_SCROLL_SPEED).toInt();
}

void SettingsManager::setGridScrollSpeed(int speed) {
    writeSetting(KEY_GRID_SCROLL_SPEED, speed,
                 &SettingsManager::gridScrollSpeedChanged);
}

QRect SettingsManager::windowGeometry() const {
    return settings.value(KEY_WINDOW_RECT, QRect()).toRect();
}

void SettingsManager::setWindowGeometry(const QRect &geometry) {
    writeSetting(KEY_WINDOW_RECT, geometry,
                 &SettingsManager::windowGeometryChanged);
}

bool SettingsManager::hasWindowGeometry() const {
    return settings.contains(KEY_WINDOW_RECT);
}

QVariantMap SettingsManager::defaultValues() const {
    QVariantMap values;
    values.insert(QStringLiteral("thumbnailWidth"), DEFAULT_THUMB_SIZE.width());
    values.insert(QStringLiteral("thumbnailHeight"), DEFAULT_THUMB_SIZE.height());
    values.insert(QStringLiteral("viewerWidth"), DEFAULT_VIEWER_SIZE.width());
    values.insert(QStringLiteral("viewerHeight"), DEFAULT_VIEWER_SIZE.height());

    for (const SettingDef &def : settingDefs())
        values.insert(QString::fromLatin1(def.qml_name), def.default_value);

    return values;
}

void SettingsManager::resetToDefaults() {
    const bool had_window_geometry = hasWindowGeometry();
    const QRect saved_geometry = windowGeometry();

    settings.clear();
    if (had_window_geometry)
        settings.setValue(KEY_WINDOW_RECT, saved_geometry);
    settings.sync();

    for (const SettingDef &def : settingDefs())
        (this->*def.notify)();
    emit windowGeometryChanged();
}

void SettingsManager::saveWindowGeometry(int x, int y, int width, int height) {
    setWindowGeometry(QRect(x, y, width, height));
}

void SettingsManager::beginUpdate() {
    ++update_depth;
}

void SettingsManager::endUpdate() {
    if (update_depth > 0 && --update_depth == 0)
        settings.sync();
}
