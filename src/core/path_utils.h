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

    // fromLocalPath() is kept for compatibility: it accepts file:// and
    // content:// URIs unchanged and wraps everything else as a local file URL.
    Q_INVOKABLE QUrl fromLocalPath(const QString &path) const;
    // Scheme-aware input conversion for QML text fields.
    Q_INVOKABLE QUrl fromUserInput(const QString &text) const;
    // Local path on desktop, the full URL string for content:// URIs.
    Q_INVOKABLE QString toLocalPath(const QUrl &url) const;
    // Rename-dialog prefill: file name without its final extension, keeping
    // inner dots ("cat.funny.gif" → "cat.funny").
    Q_INVOKABLE QString baseName(const QString &fileName) const;

private:
    explicit PathUtils(QObject *parent = nullptr);
};

#endif // PATH_UTILS_H
