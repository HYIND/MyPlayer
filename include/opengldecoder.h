#ifndef OPENGLDECODER_H
#define OPENGLDECODER_H

//QOpenglWidget调用的ffmpeg视频解码器

#include <QString>
#include <QThread>

#include "header.h"

class opengldecoder :public QThread{
    Q_OBJECT
public :
    opengldecoder();
    ~opengldecoder();
    void seturl(QString url);
    int open_file();
    void stop();
    void seek(int position);

    void setVpts(qint64 *ptr);
    void setApts(qint64 *ptr);
    void setAudioTimeBase(AVRational base);

    int get_width();
    int get_height();

protected:
    void run();

signals:
    void sigFirst(uchar*p ,int w,int h);
    void newFrame();

private:
    AVFormatContext* fmtCtx = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codecCtx=NULL;
    AVPacket *pkt=NULL;
    AVFrame *yuvFrame=NULL;

    unsigned char *out_buffer=nullptr;

    int videoStreamIndex=-1;
    int numBytes=-1;

    QString _url;

    int width,height;

    bool stop_flag=false;

    bool is_first=true;

    qint64* pVpts=NULL;
    qint64* pApts=NULL;

    AVRational videoTimeBase;
    AVRational audioTimeBase;
};


#endif // OPENGLDECODER_H
