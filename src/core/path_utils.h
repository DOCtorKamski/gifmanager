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

    Q_INVOKABLE QUrl fromLocalPath(const QString &path) const;
    Q_INVOKABLE QString toLocalPath(const QUrl &url) const;
    Q_INVOKABLE QString baseName(const QString &fileName) const;

private:
    explicit PathUtils(QObject *parent = nullptr);
};

#endif // PATH_UTILS_H
