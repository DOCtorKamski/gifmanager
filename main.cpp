#include "gif_controller.h"
#include "settings.h"
#include "thumbnail_provider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("softBy_DOCtorKamski");
    app.setApplicationName("gifmanager");

    auto *thumbnail_provider = new ThumbnailProvider;
    GifController controller(thumbnail_provider);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("gifthumb"), thumbnail_provider);
    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"),
                                             &SettingsManager::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("gifController"), &controller);

    QObject::connect(&app, &QGuiApplication::aboutToQuit,
                     &controller, &GifController::shutdown);

    engine.loadFromModule("gifmanager", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
