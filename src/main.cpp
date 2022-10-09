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


extern "C"
{

    #include "SDL.h"
    #undef main //
}


int main(int argc, char *argv[])
{
    //SDLTEST
//    if(SDL_Init(SDL_INIT_VIDEO) == -1){	//SDL_初始化
//        printf("Could not initialize SDL!\n");
//        return 0;
//    }
//    printf("SDL initialized.\n");
//    SDL_Quit();	//退出SDL调用


    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.setWindowTitle("MyPlayer");
    return a.exec();
}

