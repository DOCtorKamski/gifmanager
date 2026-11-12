#include "gif_file_operations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

// NOTE: these helpers still operate on local files only. Step 2 of the Android
// preparation moves delete/rename into GifStorage so a SAF-backed backend can
// replace them without touching the controller.
QString GifFileOperations::validateFile(const QUrl &url)
{
    if (!url.isLocalFile())
        return tr("Unsupported location.");
    if (!QFileInfo::exists(url.toLocalFile()))
        return tr("File does not exist.");
    return {};
}

GifFileOperations::RenamePlan GifFileOperations::planRename(
    const QUrl &url, const QString &new_base_name)
{
    RenamePlan plan;

    if (!url.isLocalFile()) {
        plan.error = tr("Unsupported location.");
        return plan;
    }

    const QFileInfo file_info(url.toLocalFile());
    const QString old_name = file_info.baseName();
    const QString trimmed_name = new_base_name.trimmed();

    if (trimmed_name.isEmpty()) {
        plan.error = tr("Name cannot be empty.");
        return plan;
    }
    if (trimmed_name == old_name)
        return plan;

    if (trimmed_name.contains(QRegularExpression("[/\\\\:*?\"<>|]"))) {
        plan.error = tr("Name contains invalid characters.");
        return plan;
    }

    const QString extension = file_info.suffix();
    const QString new_path = QDir(file_info.absolutePath()).filePath(
        extension.isEmpty() ? trimmed_name : trimmed_name + QLatin1Char('.') + extension);

    const QUrl new_url = QUrl::fromLocalFile(new_path);
    if (QFile::exists(new_path)) {
        plan.error = tr("A file with this name already exists.");
        return plan;
    }

    plan.new_url = new_url;
    plan.changed = true;
    return plan;
}

QString GifFileOperations::removeFile(const QUrl &url)
{
    if (!url.isLocalFile())
        return tr("Unsupported location.");

    QFile file(url.toLocalFile());
    if (!file.remove())
        return tr("Failed to delete file: %1").arg(QFileInfo(url.toLocalFile()).fileName());
    return {};
}

QString GifFileOperations::renameFile(const QUrl &url, const QUrl &new_url)
{
    if (!url.isLocalFile() || !new_url.isLocalFile())
        return tr("Unsupported location.");

    if (!QFile::rename(url.toLocalFile(), new_url.toLocalFile()))
        return tr("Failed to rename file.");
    return {};
}
