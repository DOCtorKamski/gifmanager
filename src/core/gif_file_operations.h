#ifndef GIF_FILE_OPERATIONS_H
#define GIF_FILE_OPERATIONS_H

#include <QCoreApplication>
#include <QString>

class GifFileOperations
{
    Q_DECLARE_TR_FUNCTIONS(GifFileOperations)
public:
    struct RenamePlan {
        QString error;
        QString new_path;
        bool changed = false;
    };

    static QString validateFile(const QString &path);
    static RenamePlan planRename(const QString &path, const QString &new_base_name);
    static QString removeFile(const QString &path);
    static QString renameFile(const QString &path, const QString &new_path);
};

#endif // GIF_FILE_OPERATIONS_H
