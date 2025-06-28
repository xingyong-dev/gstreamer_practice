//
// Created by sylar on 25-6-18.
//

#include <QHBoxLayout>
#include <QTimer>
#include "player_window.h"
PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent){

    videoArea = new QWidget(this);
    videoArea->setAttribute(Qt::WA_NativeWindow);
    videoArea->setMinimumSize(352,288);

    btnPlay = new QPushButton("播放", this);
    btnPause = new QPushButton("暂停", this);
    btnStop = new QPushButton("停止", this);

    connect(btnPlay,&QPushButton::clicked , this, &PlayerWindow::onPlay);
    connect(btnPause, &QPushButton::clicked, this, &PlayerWindow::onpause);
    connect(btnStop , &QPushButton::clicked, this, &PlayerWindow::onstop);

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(btnPlay);
    buttonLayout->addWidget(btnPause);
    buttonLayout->addWidget(btnStop);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(videoArea);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

//    QTimer *timer = new QTimer(this);
//    connect(timer,&QTimer::timeout,this, [=](){
//        std::lock_guard<std::mutex> lock(frame_mutex);
//        if(!lastest_frame.empty())
//        {
//            cv::imshow("Video", lastest_frame);
//            cv::waitKey(1);
//        }
//    });
//    timer->start(30);
}

PlayerWindow::~PlayerWindow() noexcept {
    player.stop();
}


void PlayerWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void PlayerWindow::onPlay() {
    player.play(rtspUrl, videoArea->winId());
}

void PlayerWindow::onpause() {
    player.pause();
}

void PlayerWindow::onstop() {
    player.stop();
}
