#ifndef GIF_MODEL_H
#define GIF_MODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QStringList>
#include <QUrl>
#include <QVector>

class ThumbnailProvider;

class GifModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        FileUrlRole,
        ThumbnailUrlRole,
        ThumbnailReadyRole
    };

    explicit GifModel(ThumbnailProvider *provider, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void resetFiles(const QStringList &paths);
    void setThumbnail(const QString &path, const QImage &image);
    bool removeFile(const QString &path);

private:
    struct Item {
        QString path;
        QString file_name;
        QString image_id;
        quint64 revision = 0;
        bool thumbnail_ready = false;
    };

    QString makeImageId(const Item &item) const;
    void rebuildPathIndex();

    ThumbnailProvider *thumbnail_provider = nullptr;
    QVector<Item> items;
    QHash<QString, int> path_to_row;
};

#endif // GIF_MODEL_H
