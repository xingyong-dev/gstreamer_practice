#include <QApplication>
#include "player_window.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    PlayerWindow window;
    window.show();

//    window.startStreaming("rtsp://admin:12345@192.168.0.200:554/streaming/Channels/102");

    return app.exec();
}
