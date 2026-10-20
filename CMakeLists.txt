cmake_minimum_required(VERSION 3.19)
project(gifmanager LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    QuickLayouts
)

qt_standard_project_setup()

qt_add_executable(gifmanager
    WIN32 MACOSX_BUNDLE
    main.cpp

    gif_controller.h gif_controller.cpp
    gif_model.h gif_model.cpp
    gif_loader_worker.h gif_loader_worker.cpp
    thumbnail_provider.h thumbnail_provider.cpp
    settings.h settings.cpp
)

qt_add_qml_module(gifmanager
    URI gifmanager
    VERSION 1.0
    QML_FILES
        Main.qml
        GifDelegate.qml
        GifViewerWindow.qml
        SettingsDialog.qml
        ConfirmDeleteDialog.qml
        RenameGifDialog.qml
        ErrorDialog.qml
)

target_include_directories(gifmanager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gifmanager
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickDialogs2
        Qt::QuickLayouts
)

include(GNUInstallDirs)

install(TARGETS gifmanager
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET gifmanager
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
