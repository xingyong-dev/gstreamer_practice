#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "gst_player.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
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
private:

    std::string rtspUrl = "rtsp://admin:12345@192.168.0.200:554/streaming/Channels/102";
};
#endif // MAINWINDOW_H
