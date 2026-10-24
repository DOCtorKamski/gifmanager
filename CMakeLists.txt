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

# engine.loadFromModule() resolves qrc:/qt/qml/<uri>/<type>.qml. Without this
# policy the QML module is built with the legacy :/<uri>/ resource prefix and
# startup fails with 'Module "gifmanager" contains no type named "Main"'.
qt_policy(SET QTP0001 NEW)

include(CTest)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------
# Everything that does not depend on main(). Keeping it in a library lets the
# tests link against the backend without the application target.

qt_add_library(gifmanager_core STATIC
    src/core/gif_controller.h src/core/gif_controller.cpp
    src/core/gif_model.h src/core/gif_model.cpp
    src/core/gif_loader_task.h src/core/gif_loader_task.cpp
    src/core/gif_loader_worker.h src/core/gif_loader_worker.cpp
    src/core/thumbnail_provider.h src/core/thumbnail_provider.cpp
    src/core/settings.h src/core/settings.cpp
)

# Public so consumers can keep using #include "gif_controller.h" and friends.
target_include_directories(gifmanager_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(gifmanager_core
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------

qt_add_executable(gifmanager
    WIN32 MACOSX_BUNDLE
    src/main.cpp
)

target_link_libraries(gifmanager
    PRIVATE
        gifmanager_core
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickDialogs2
        Qt::QuickLayouts
)

# QML sources live in qml/ but belong to the root of the module, otherwise
# engine.loadFromModule("gifmanager", "Main") would look for
# qrc:/qt/qml/gifmanager/qml/Main.qml.
set(GIFMANAGER_QML_FILES
    qml/Main.qml
    qml/GifDelegate.qml
    qml/GifViewerWindow.qml
    qml/SettingsDialog.qml
    qml/ConfirmDeleteDialog.qml
    qml/RenameGifDialog.qml
    qml/ErrorDialog.qml
)

foreach(qml_file IN LISTS GIFMANAGER_QML_FILES)
    get_filename_component(qml_alias "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qml_alias}"
    )
endforeach()

# The module's generated files (qmldir, qmltypes, copied QML sources) are
# written into <QT_QML_OUTPUT_DIRECTORY>/<URI path>. With the default output
# directory that would be <build>/gifmanager, i.e. the same path as the
# gifmanager executable, and linking would fail with
# "cannot open output file gifmanager: Is a directory".
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml")

qt_add_qml_module(gifmanager
    URI gifmanager
    VERSION 1.0
    QML_FILES ${GIFMANAGER_QML_FILES}
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

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
