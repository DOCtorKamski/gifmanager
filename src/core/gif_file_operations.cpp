#include "gif_file_operations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

QString GifFileOperations::validateFile(const QString &path)
{
    if (!QFileInfo::exists(path))
        return tr("File does not exist.");
    return {};
}

GifFileOperations::RenamePlan GifFileOperations::planRename(
    const QString &path, const QString &new_base_name)
{
    RenamePlan plan;

    const QFileInfo file_info(path);
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
    plan.new_path = QDir(file_info.absolutePath()).filePath(
        extension.isEmpty() ? trimmed_name : trimmed_name + QLatin1Char('.') + extension);

    if (QFile::exists(plan.new_path)) {
        plan.error = tr("A file with this name already exists.");
        return plan;
    }

    plan.changed = true;
    return plan;
}

QString GifFileOperations::removeFile(const QString &path)
{
    QFile file(path);
    if (!file.remove())
        return tr("Failed to delete file: %1").arg(QFileInfo(path).fileName());
    return {};
}

QString GifFileOperations::renameFile(const QString &path, const QString &new_path)
{
    if (!QFile::rename(path, new_path))
        return tr("Failed to rename file.");
    return {};
}
