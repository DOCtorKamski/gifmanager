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

QUrl PathUtils::fromLocalPath(const QString &path) const
{
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QString PathUtils::toLocalPath(const QUrl &url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString PathUtils::baseName(const QString &fileName) const
{
    return QFileInfo(fileName).baseName();
}
