#include "clipboard_service.h"
#include "gif_controller.h"
#include "path_utils.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <qqml.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(AppIdentity::ORGANIZATION);
    app.setApplicationName(AppIdentity::APPLICATION);

    auto *thumbnail_provider = new ThumbnailProvider;
    GifController controller(thumbnail_provider);

    qmlRegisterSingletonInstance("gifmanager", 1, 0, "AppSettings",
                                 &SettingsManager::instance());
    qmlRegisterSingletonInstance("gifmanager", 1, 0, "PathUtils",
                                 &PathUtils::instance());
    qmlRegisterSingletonInstance("gifmanager", 1, 0, "ClipboardService",
                                 &ClipboardService::instance());
    qmlRegisterSingletonInstance("gifmanager", 1, 0, "GifController",
                                 &controller);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("gifthumb"), thumbnail_provider);

    QObject::connect(&app, &QGuiApplication::aboutToQuit,
                     &controller, &GifController::shutdown);

    engine.loadFromModule("gifmanager", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
