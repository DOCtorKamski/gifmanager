#ifndef GIF_STORAGE_FACTORY_H
#define GIF_STORAGE_FACTORY_H

#include <memory>

class GifStorage;

// Returns the storage backend for the current platform: SAF on Android and
// QDir/QFile on desktop.
std::shared_ptr<GifStorage> createPlatformGifStorage();

#endif // GIF_STORAGE_FACTORY_H
