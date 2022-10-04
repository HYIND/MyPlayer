#include "opengldecoder.h"
#include <QDateTime>
#include <QDebug>
#include <windows.h>
//QOpenglWidget调用的ffmpeg视频解码器


opengldecoder::opengldecoder()
{
    this->fmtCtx=avformat_alloc_context();
    this->pkt=av_packet_alloc();
    this->yuvFrame=av_frame_alloc();
}

opengldecoder::~opengldecoder()
{
    if(pkt) av_packet_free(&pkt);
    if(yuvFrame) av_frame_free(&yuvFrame);
    if(codecCtx) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
    }
    if(fmtCtx) avformat_free_context(fmtCtx);
}

void opengldecoder::seturl(QString url)
{
    _url=url;
}

int opengldecoder::open_file()
{
    if(_url.isEmpty()) return -1;

    if(avformat_open_input(&fmtCtx,_url.toStdString().c_str(),NULL,NULL)<0) return -1;

    if(avformat_find_stream_info(fmtCtx,NULL)<0) return -1;

    int streamCnt=fmtCtx->nb_streams;
    for (int i=0; i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
            videoStreamIndex=i;
            continue;
        }
    }

    if(videoStreamIndex==-1) return -1;

    AVCodecParameters* codecPara =fmtCtx->streams[videoStreamIndex]->codecpar;

    if(!(codec=avcodec_find_decoder(codecPara->codec_id))) return -1;

    if(!(codecCtx=avcodec_alloc_context3(codec))) return -1;

    if(avcodec_parameters_to_context(codecCtx,codecPara)) return -1;

    if(avcodec_open2(codecCtx,codec,NULL)<0)return -1;

    width=codecCtx->width;
    height=codecCtx->height;

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,width,height,1);

    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(unsigned char));

    return true;
}

void opengldecoder::stop()
{
    stop_flag=true;
}

int opengldecoder::get_height()
{
    return this->height;
}

int opengldecoder::get_width()
{
    return this->width;
}

void opengldecoder::run()
{
    stop_flag=false;
    if(open_file()==-1 )return;
    emit sigFirst(out_buffer,width,height);

//    timeBeginPeriod(1);
    double timeInOneFps = 1.0 / (fmtCtx->streams[videoStreamIndex]->avg_frame_rate.num/fmtCtx->streams[videoStreamIndex]->avg_frame_rate.den); // 每秒60帧，则1帧就是约16毫秒
//    int ms_perframe=1000/(fmtCtx->streams[videoStreamIndex]->avg_frame_rate.num/fmtCtx->streams[videoStreamIndex]->avg_frame_rate.den);

//    qint64 goal=QDateTime::currentDateTime().toMSecsSinceEpoch()+ms_perframe;
//    qint64 now=0;

    _LARGE_INTEGER time_now;
    _LARGE_INTEGER time_last;
    double dqFreq;		//计时器频率
    LARGE_INTEGER f;	//计时器频率
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart;
    QueryPerformanceCounter(&time_last);//获取计时器跳数
    int i = 0;

    while(av_read_frame(fmtCtx,pkt)>=0&&!stop_flag){
        if(pkt->stream_index==videoStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                int ret;
                int bytes=0;
                while((ret=avcodec_receive_frame(codecCtx,yuvFrame))>=0&&!stop_flag){
                    QueryPerformanceCounter(&time_now);
                    while ((time_now.QuadPart - time_last.QuadPart) / dqFreq < timeInOneFps)
                    {
                        QueryPerformanceCounter(&time_now);
                        i = (timeInOneFps - (time_now.QuadPart - time_last.QuadPart) / dqFreq) * 1000;
                        if (i > 0)
                            Sleep(i);
                    }
                    time_last = time_now;

//                    now=QDateTime::currentDateTime().toMSecsSinceEpoch();
//                    if(now<goal) QThread::msleep(goal-now);
//                    goal+=ms_perframe;
                    if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF) return;
                    else if(ret<0) continue;

                    for(int i=0;i<height;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[0]+yuvFrame->linesize[0]*i,width);
                        bytes+=width;
                    }
                    int uv=height>>1;
                    for(int i=0;i<uv;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[1]+yuvFrame->linesize[1]*i,width/2);
                        bytes+=width/2;
                    }
                    for(int i=0;i<uv;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[2]+yuvFrame->linesize[2]*i,width/2);
                        bytes+=width/2;
                    }
                    bytes=0;
                    emit newFrame();
                }
            }
            av_packet_unref(pkt);
        }
    }
    //    if(yuvFrame) av_frame_free(&yuvFrame);
    //    if(rgbFrame) av_frame_free(&rgbFrame);
    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    codecCtx=NULL;
    codec=NULL;
    avformat_free_context(fmtCtx);
    fmtCtx=NULL;
}
