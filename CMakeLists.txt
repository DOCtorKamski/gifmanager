cmake_minimum_required(VERSION 3.19)
project(gifmanager LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(gifmanager
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    clickable_label.h clickable_label.cpp
    gif_thumbnail_grid.h gif_thumbnail_grid.cpp
    gif_viewer.h
    gif_viewer.cpp
    gif_loader_worker.h gif_loader_worker.cpp
    settings.h settings.cpp
    settings_dialog.h settings_dialog.cpp
)

target_include_directories(gifmanager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gifmanager
    PRIVATE
        Qt::Core
        Qt::Widgets
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
