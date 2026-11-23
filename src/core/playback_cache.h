#ifndef PLAYBACK_CACHE_H
#define PLAYBACK_CACHE_H

#include <QByteArray>
#include <QString>
#include <QUrl>

// Per-session cache of document bytes that QML can play as file:// URLs.
// AnimatedImage cannot open content:// documents, so the Android backend copies
// each GIF into this cache and serves the local copy. Files are named after a
// hash of the source URL, which keeps them stable for a session and distinct
// for renamed or re-created documents.
//
// The class is platform-independent so it can be unit tested on desktop; it is
// only used by the Android storage backend.
class PlaybackCache
{
public:
    // Clears leftovers from the previous session: deleted or renamed documents
    // must not keep serving stale content.
    PlaybackCache();

    // Local file URL for source, or an empty URL when nothing is cached yet.
    QUrl cachedUrl(const QUrl &source) const;

    // Copies data into the cache and returns the file:// URL, or an empty URL
    // when the file cannot be written. Storing an already cached source keeps
    // the existing copy.
    QUrl store(const QUrl &source, const QByteArray &data);

    // Removes every cached file. Called from the constructor.
    void clear() const;

    static QString directory();
    static QString filePath(const QUrl &source);
};

#endif // PLAYBACK_CACHE_H
