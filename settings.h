#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QSize>
#include <QString>

class SettingsManager {
public:
    SettingsManager();

    QSize getThumbnailSize() const;
    int getColumns() const;

    void setThumbnailSize(const QSize &size);
    void setColumns(int columns);

    QString getDefaultFolder() const;
    void setDefaultFolder(const QString &path);

    void resetToDefaults();

private:
    QSettings settings;

    static constexpr int DEFAULT_THUMB_WIDTH = 160;
    static constexpr int DEFAULT_THUMB_HEIGHT = 120;
    static constexpr int DEFAULT_COLUMNS = 3;
};

#endif // SETTINGS_H
