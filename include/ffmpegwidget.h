#ifndef FFMPEGWIDGET_H
#define FFMPEGWIDGET_H

#include <QThread>
#include <QWidget>
#include <QString>
#include <QPainter>

extern "C" {
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
}

class FFmpegVideo;

class FFmpegwidget : public QWidget
{
    Q_OBJECT
public:
    explicit FFmpegwidget(QWidget *parent = nullptr);
    ~FFmpegwidget();

    void seturl(QString url);

    void play(QString url);
    void stop();

protected:
    void paintEvent(QPaintEvent* event);

private slots:
    void recviveQImage(const QImage& rImg);

private:

    FFmpegVideo* ffmpeg;
    QImage img;
};


class FFmpegVideo :public QThread{
    Q_OBJECT
public :
    FFmpegVideo();
    ~FFmpegVideo();
    void seturl(QString url);
    int open_file();
    void stop();

protected:
    void run();

signals:
    void sendQImage(QImage);
private:
    AVFormatContext* fmtCtx = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codecCtx=NULL;
    AVPacket *pkt=NULL;
    AVFrame *yuvFrame=NULL;
    AVFrame *rgbFrame=NULL;

    SwsContext* img_ctx=NULL;

    unsigned char *out_buffer=nullptr;

    int videoStreamIndex=-1;
    int numBytes=-1;

    QString _url;

    bool stop_flag=false;
};

#endif // FFMPEGWIDGET_H
