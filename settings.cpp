#include "settings.h"

SettingsManager::SettingsManager()
    : settings("gifmanager", "gifmanager")
{
}

QSize SettingsManager::getThumbnailSize() const {
    int w = settings.value("thumbnail/width", DEFAULT_THUMB_WIDTH).toInt();
    int h = settings.value("thumbnail/height", DEFAULT_THUMB_HEIGHT).toInt();
    return QSize(w, h);
}

void SettingsManager::setThumbnailSize(const QSize &size) {
    settings.setValue("thumbnail/width", size.width());
    settings.setValue("thumbnail/height", size.height());
    settings.sync();
}

int SettingsManager::getColumns() const {
    return settings.value("layout/columns", DEFAULT_COLUMNS).toInt();
}

void SettingsManager::setColumns(int columns) {
    settings.setValue("layout/columns", columns);
    settings.sync();
}

QString SettingsManager::getDefaultFolder() const {
    return settings.value("folder/default", "").toString();
}

void SettingsManager::setDefaultFolder(const QString &path) {
    settings.setValue("folder/default", path);
    settings.sync();
}

void SettingsManager::resetToDefaults() {
    settings.clear();
    settings.sync();
}
