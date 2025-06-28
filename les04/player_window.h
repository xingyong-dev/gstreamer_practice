//
// Created by sylar on 25-6-18.
//

#ifndef LES04_PLAYER_WINDOW_H
#define LES04_PLAYER_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include "gst_player.h"



class PlayerWindow : public QWidget {

    Q_OBJECT
public:
    PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow();

//    void startStreaming(const std::string& uri);
//    void stopStreaming();
private slots:
    void onPlay();
    void onpause();
    void onstop();

protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    GstPlayer player;
    QWidget *videoArea;
    QPushButton *btnPlay;
    QPushButton *btnPause;
    QPushButton *btnStop;
    std::string rtspUrl = "rtsp://admin:12345@192.168.0.200:554/streaming/Channels/102";
};


#endif //LES04_PLAYER_WINDOW_H
