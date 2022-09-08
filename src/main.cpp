#include "mainwindow.h"

#include <QApplication>


extern "C"
{

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/version.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"

#undef main

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    qDebug()<<"this test ffmpeg!";
//    av_register_all();
    unsigned version = avcodec_version();
    qDebug()<<"ffmpeg version:"<<version;

    MainWindow w;
//111111111111111111111

    w.show();
    return a.exec();
}
