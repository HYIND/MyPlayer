#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

//ffmpeg解码器，待用

#include <QString>
#include <QThread>
#include <QImage>

#include "header.h"

class FFmpegdecoder :public QThread{
    Q_OBJECT
public :
    FFmpegdecoder();
    ~FFmpegdecoder();
    void seturl(QString url);
    int open_file();
    void stop();
    int hw_decoder_init(AVCodecContext *ctx, const AVHWDeviceType type);
//    AVPixelFormat get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts);

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
    AVFrame *nv12Frame=NULL;
    AVFrame *rgbFrame=NULL;

    SwsContext* img_ctx=NULL;

    unsigned char *out_buffer=nullptr;

    int videoStreamIndex=-1;
    int numBytes=-1;

    QString _url;


    bool stop_flag=false;
};


#endif // FFMPEGDECODER_H
