//
// Created by sylar on 25-6-12.
//

#ifndef LES04_GST_PLAYER_H
#define LES04_GST_PLAYER_H
#include <mutex>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <QObject>
#include <QWidget>
#include <opencv2/opencv.hpp>

extern std::mutex frame_mutex;
extern cv::Mat lastest_frame;

class GstPlayer : public QObject{
    Q_OBJECT
public:
    explicit GstPlayer(QObject *parent = nullptr);
    ~GstPlayer();

    void play(const std::string& url, WId videoWinId);
    void pause();
    void stop();
private:
    GstElement *pipeline = nullptr;
};


#endif //LES04_GST_PLAYER_H
