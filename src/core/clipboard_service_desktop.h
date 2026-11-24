#ifndef CLIPBOARD_SERVICE_DESKTOP_H
#define CLIPBOARD_SERVICE_DESKTOP_H

#include <QUrl>

class QMimeData;

// Builds the desktop clipboard payload for a GIF: FileNameW, decoded image
// data and text/uri-list. Returns nullptr for URLs that are not local files.
// Kept separate from ClipboardService::copyFile() so the payload can be unit
// tested without a system clipboard.
QMimeData *createDesktopClipboardMimeData(const QUrl &url);

#endif // CLIPBOARD_SERVICE_DESKTOP_H
