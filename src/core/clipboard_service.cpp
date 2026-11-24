#include "clipboard_service.h"

// Platform-neutral part of the QML singleton. The copyFile()/shareFile()
// implementations live in clipboard_service_desktop.cpp and
// clipboard_service_android.cpp; CMake compiles exactly one of them.

ClipboardService &ClipboardService::instance()
{
    static ClipboardService inst;
    return inst;
}

ClipboardService::ClipboardService(QObject *parent)
    : QObject(parent)
{
}
