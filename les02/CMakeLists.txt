cmake_minimum_required(VERSION 3.24)
project(les02)
set(CMAKE_CXX_STANDARD 17)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

include_directories(

        ${GST_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
)
add_definitions(${GST_CFLAGS_TOHER})

add_executable(les02 main.cpp)
target_link_libraries(les02
        ${GST_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
        tensorflow-lite
        opencv_core
        opencv_highgui
        opencv_imgproc
        opencv_videoio   # 如需打开本地摄像头或视频文件
        opencv_imgcodecs # 如需保存图片到本地
)
