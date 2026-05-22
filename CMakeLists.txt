cmake_minimum_required(VERSION 3.16)
project(LMAO VERSION 1.0.0 LANGUAGES CXX)
add_definitions(-DAPP_VERSION="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# DBus is Linux-only, use Network module on Windows for single-instance
if(UNIX AND NOT APPLE)
    find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets DBus)
    set(PLATFORM_LIBS Qt6::DBus)
    add_definitions(-DUSE_DBUS)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Network)
    set(PLATFORM_LIBS Qt6::Network)
endif()

# mpv: pkg-config on Linux, manual find on Windows
if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED mpv)
    include(GNUInstallDirs)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Network)
    set(PLATFORM_LIBS Qt6::Network opengl32)
    set(MPV_DIR "${CMAKE_SOURCE_DIR}/thirdparty/mpv" CACHE PATH "Path to mpv SDK")
    set(MPV_INCLUDE_DIRS "${MPV_DIR}/include")
    set(MPV_LIBRARIES "${MPV_DIR}/libmpv.dll.a")
    set(MPV_LIBRARY_DIRS "${MPV_DIR}")
endif()

add_executable(lmao
    src/main.cpp
    src/mpvwidget.h
    src/mpvwidget.cpp
    src/mpvcontroller.h
    src/mpvcontroller.cpp
    src/playerwindow.h
    src/playerwindow.cpp
    src/widgets/customslider.h
    src/widgets/customslider.cpp
    src/widgets/glasspanel.h
    src/widgets/glasspanel.cpp
    $<$<PLATFORM_ID:Windows>:src/app.rc>
)

target_include_directories(lmao PRIVATE ${MPV_INCLUDE_DIRS})
target_link_libraries(lmao PRIVATE Qt6::Widgets Qt6::OpenGLWidgets ${PLATFORM_LIBS} ${MPV_LIBRARIES})
target_link_directories(lmao PRIVATE ${MPV_LIBRARY_DIRS})

# Windows: copy DLLs next to exe
if(WIN32)
    set_target_properties(lmao PROPERTIES WIN32_EXECUTABLE TRUE)
    add_custom_command(TARGET lmao POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MPV_DIR}/libmpv-2.dll" $<TARGET_FILE_DIR:lmao>)
    # Copy ffprobe/ffmpeg if they exist in thirdparty
    foreach(TOOL ffprobe.exe ffmpeg.exe)
        if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/${TOOL}")
            add_custom_command(TARGET lmao POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/thirdparty/${TOOL}" $<TARGET_FILE_DIR:lmao>)
        endif()
    endforeach()
endif()

# Linux install targets
if(UNIX AND NOT APPLE)
    install(TARGETS lmao DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES dev.elek.LMAO.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES dev.elek.LMAO.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES icons/hicolor/16x16/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps)
    install(FILES icons/hicolor/32x32/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
    install(FILES icons/hicolor/48x48/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps)
    install(FILES icons/hicolor/64x64/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps)
    install(FILES icons/hicolor/128x128/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps)
    install(FILES icons/hicolor/256x256/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    install(FILES icons/hicolor/512x512/apps/dev.elek.LMAO.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
    install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME})
endif()