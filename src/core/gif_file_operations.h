#ifndef GIF_FILE_OPERATIONS_H
#define GIF_FILE_OPERATIONS_H

#include <QCoreApplication>
#include <QString>
#include <QUrl>

class GifFileOperations
{
    Q_DECLARE_TR_FUNCTIONS(GifFileOperations)
public:
    struct RenamePlan {
        QString error;
        QUrl new_url;
        bool changed = false;
    };

    static QString validateFile(const QUrl &url);
    static RenamePlan planRename(const QUrl &url, const QString &new_base_name);
    static QString removeFile(const QUrl &url);
    static QString renameFile(const QUrl &url, const QUrl &new_url);
};

#endif // GIF_FILE_OPERATIONS_H
