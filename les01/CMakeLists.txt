cmake_minimum_required(VERSION 3.24)
project(rtsp_stream_saver)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

# 可选：针对 i.MX8MP 添加 G2D 和硬件加速库（通常都通过 gstreamer plugins 已提供）
# 不单独添加 imx 插件库路径，依赖 gstreamer 的插件机制自动加载

include_directories(
        ${GST_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
)

add_definitions(${GST_CFLAGS_OTHER})

add_executable(rtsp_stream_saver main.cpp)

target_link_libraries(rtsp_stream_saver
        ${GST_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
)