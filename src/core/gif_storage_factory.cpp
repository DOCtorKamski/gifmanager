#include "gif_storage_factory.h"

#include "desktop_gif_storage.h"

#ifdef Q_OS_ANDROID
#include "android_gif_storage.h"
#endif

std::shared_ptr<GifStorage> createPlatformGifStorage()
{
#ifdef Q_OS_ANDROID
    return std::make_shared<AndroidGifStorage>();
#else
    return std::make_shared<DesktopGifStorage>();
#endif
}
