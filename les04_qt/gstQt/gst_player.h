#ifndef GST_PLAYER_H
#define GST_PLAYER_H
#include <mutex>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <QObject>
#include <QWidget>


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


#endif // GST_PLAYER_H
