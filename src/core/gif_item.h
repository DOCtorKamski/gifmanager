#ifndef GIF_ITEM_H
#define GIF_ITEM_H

#include <QMetaType>
#include <QString>
#include <QUrl>

// A single GIF as seen by the application. `url` is the stable identity
// (file:// on desktop, content:// once Android storage lands) and `fileName`
// is the display name supplied by the storage backend.
struct GifItem
{
    QUrl url;
    QString fileName;
};

Q_DECLARE_METATYPE(GifItem)

#endif // GIF_ITEM_H
