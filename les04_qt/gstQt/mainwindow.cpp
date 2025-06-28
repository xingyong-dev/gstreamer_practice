#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gst/gst.h"
#include <QHBoxLayout>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)

{
        videoArea = new QWidget(this);
        videoArea->setAttribute(Qt::WA_NativeWindow);
        videoArea->setMinimumSize(352,288);

        btnPlay = new QPushButton("播放", this);
        btnPause = new QPushButton("暂停", this);
        btnStop = new QPushButton("停止", this);

        connect(btnPlay,&QPushButton::clicked , this, &MainWindow::onPlay);
        connect(btnPause, &QPushButton::clicked, this, &MainWindow::onpause);
        connect(btnStop , &QPushButton::clicked, this, &MainWindow::onstop);

        auto buttonLayout = new QHBoxLayout;
        buttonLayout->addWidget(btnPlay);
        buttonLayout->addWidget(btnPause);
        buttonLayout->addWidget(btnStop);

        auto mainLayout = new QVBoxLayout;
        mainLayout->addWidget(videoArea);
        mainLayout->addLayout(buttonLayout);
        setLayout(mainLayout);
        player.play(rtspUrl, videoArea->winId());
}


MainWindow::~MainWindow()  {
    player.stop();
}


void MainWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void MainWindow::onPlay() {
    player.play(rtspUrl, videoArea->winId());
}

void MainWindow::onpause() {
    player.pause();
}

void MainWindow::onstop() {
    player.stop();
}

