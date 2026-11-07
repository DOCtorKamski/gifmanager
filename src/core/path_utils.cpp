#include "path_utils.h"

#include <QFileInfo>

PathUtils &PathUtils::instance()
{
    static PathUtils inst;
    return inst;
}

PathUtils::PathUtils(QObject *parent)
    : QObject(parent)
{
}

QString PathUtils::displayName(const QString &path) const
{
    return QFileInfo(path).fileName();
}

QString PathUtils::baseNameForPath(const QString &path) const
{
    return QFileInfo(path).baseName();
}

QUrl PathUtils::localFileUrl(const QString &path) const
{
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QString PathUtils::localPathFromUrl(const QUrl &url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}
