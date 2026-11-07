#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <QObject>
#include <QString>
#include <QUrl>

class PathUtils : public QObject
{
    Q_OBJECT
public:
    static PathUtils &instance();

    Q_INVOKABLE QString displayName(const QString &path) const;
    Q_INVOKABLE QString baseNameForPath(const QString &path) const;
    Q_INVOKABLE QUrl localFileUrl(const QString &path) const;
    Q_INVOKABLE QString localPathFromUrl(const QUrl &url) const;

private:
    explicit PathUtils(QObject *parent = nullptr);
};

#endif // PATH_UTILS_H
