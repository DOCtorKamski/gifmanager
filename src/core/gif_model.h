#ifndef GIF_MODEL_H
#define GIF_MODEL_H

#include "gif_item.h"

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QList>
#include <QUrl>
#include <QVector>

class ThumbnailProvider;

class GifModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FileUrlRole,
        ThumbnailUrlRole,
        ThumbnailReadyRole
    };

    explicit GifModel(ThumbnailProvider *provider, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void resetFiles(const QList<GifItem> &gif_items);
    void setThumbnail(const QUrl &url, const QImage &image);
    bool removeFile(const QUrl &url);
    bool contains(const QUrl &url) const;

private:
    struct Item {
        QUrl url;
        QString file_name;
        QString image_id;
        quint64 revision = 0;
        bool thumbnail_ready = false;
    };

    QString makeImageId(const Item &item) const;
    void rebuildUrlIndex();

    ThumbnailProvider *thumbnail_provider = nullptr;
    QVector<Item> items;
    QHash<QUrl, int> url_to_row;
    quint64 generation = 0;
};

#endif // GIF_MODEL_H
