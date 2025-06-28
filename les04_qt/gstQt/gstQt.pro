QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += link_pkgconfig
# 添加 GStreamer 所需模块
PKGCONFIG += gstreamer-1.0 \
             gstreamer-base-1.0 \
             gstreamer-video-1.0 \
             gstreamer-app-1.0


# 2. 可选手动指定（如 pkg-config 不可用时）
# INCLUDEPATH += /usr/include/gstreamer-1.0 /usr/include/glib-2.0
# LIBS += -lgstreamer-1.0 -lgstbase-1.0 -lgstvideo-1.0 -lgobject-2.0 -lglib-2.0


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gst_player.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    gst_player.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
