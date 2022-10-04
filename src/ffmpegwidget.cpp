#include "ffmpegwidget.h"
//ffmpeg播放视频类，待用

FFmpegwidget::FFmpegwidget(QWidget *parent)
    : QWidget{parent}
{
    this->ffmpeg=new FFmpegdecoder();
    connect(ffmpeg,SIGNAL(sendQImage(QImage)),this,SLOT(recviveQImage(QImage)));
    //    connect(ffmpeg,&FFmpegdecoder::finished,ffmpeg,&FFmpegdecoder::deleteLater);
}

FFmpegwidget::~FFmpegwidget()
{
    if(ffmpeg->isRunning()) stop();
}

void FFmpegwidget::seturl(QString url)
{
    ffmpeg->seturl(url);
}

void FFmpegwidget::play(QString url)
{
    stop();
    seturl(url);
    ffmpeg->start();
}

void FFmpegwidget::stop()
{
    if(ffmpeg->isRunning()){
        ffmpeg->stop();
        ffmpeg->requestInterruption();
        ffmpeg->quit();
        ffmpeg->wait(100);
    }
    img.fill(Qt::black);
}

void FFmpegwidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawImage(0,0,img);
}

void FFmpegwidget::recviveQImage(const QImage &rImg)
{
    img=rImg.scaled(this->size());
    update();
}
