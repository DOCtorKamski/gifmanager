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

QUrl PathUtils::fromUserInput(const QString &text) const
{
    if (text.isEmpty())
        return {};

    const QUrl url(text);
    if (url.scheme() == QLatin1String("file")
        || url.scheme() == QLatin1String("content")) {
        return url;
    }
    return QUrl::fromLocalFile(text);
}

QString PathUtils::toLocalPath(const QUrl &url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString PathUtils::baseName(const QString &fileName) const
{
    // completeBaseName() strips only the final extension, so "cat.funny.gif"
    // prefills the rename dialog with "cat.funny" instead of "cat".
    return QFileInfo(fileName).completeBaseName();
}
