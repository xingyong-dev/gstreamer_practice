cmake_minimum_required(VERSION 3.24)
project(les04)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


set(CMAKE_PREFIX_PATH "/home/sylar/Qt5.12.12/Tools/QtCreator/bin")

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(
        ${GST_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        ${GST_BASE_INCLUDE_DIRS}
)
add_definitions(${GST_CFLAGS_TOHER})

add_executable(les04 main.cpp gst_player.cpp gst_player.h player_window.cpp player_window.h)
target_link_libraries(les04

        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        ${GST_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
        ${GST_BASE_LIBRARIES}
        ${OpenCV_LIBS}
        )

