#include "mainwindow.h"
#include <iostream>
#include <fstream>
#include <QApplication>
using namespace std;

extern "C"
{

#include "libavfilter/avfilter.h"
#include "libswresample/swresample.h"
#include "libpostproc/postprocess.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/version.h"

#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"

#undef main

}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.setWindowTitle("MyPlayer");
    return a.exec();
}

