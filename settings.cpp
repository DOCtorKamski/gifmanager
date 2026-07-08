#include "settings.h"

SettingsManager::SettingsManager()
    : settings("gifmanager", "gifmanager")
{
}

QSize SettingsManager::getThumbnailSize() const {
    return settings.value("thumbnail/size", DEFAULT_THUMB_SIZE).toSize();
}

void SettingsManager::setThumbnailSize(const QSize &size) {
    settings.setValue("thumbnail/size", size);
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
    setThumbnailSize(DEFAULT_THUMB_SIZE);
    setColumns(DEFAULT_COLUMNS);
    setDefaultFolder(QString());
}
