find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

# All tests are headless: they exercise the backend without a QGuiApplication.
# WIN32_EXECUTABLE is forced off so console output is visible on Windows.

function(gifmanager_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE gifmanager_core Qt::Test)
    set_target_properties(${name} PROPERTIES
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE FALSE
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

gifmanager_add_test(tst_gif_model)
gifmanager_add_test(tst_gif_loader_worker)
gifmanager_add_test(tst_gif_loader_task)
gifmanager_add_test(tst_gif_library)
gifmanager_add_test(tst_desktop_gif_storage)
gifmanager_add_test(tst_gif_controller)
gifmanager_add_test(tst_settings)
gifmanager_add_test(tst_playback_cache)
gifmanager_add_test(tst_path_utils)

if(NOT ANDROID)
    # Uses an offscreen QGuiApplication because the payload embeds a QPixmap.
    gifmanager_add_test(tst_clipboard)
endif()
