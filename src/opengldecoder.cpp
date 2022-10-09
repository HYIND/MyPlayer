#include "opengldecoder.h"
#include <QDateTime>
#include <QDebug>
#include <sysinfoapi.h>
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
    AVStream* pAVStream = fmtCtx->streams[videoStreamIndex];

    if(!(codec=avcodec_find_decoder(codecPara->codec_id))) return -1;

    if(!(codecCtx=avcodec_alloc_context3(codec))) return -1;

    if(avcodec_parameters_to_context(codecCtx,codecPara)) return -1;

    if(avcodec_open2(codecCtx,codec,NULL)<0)return -1;

    width=codecCtx->width;
    height=codecCtx->height;

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,width,height,1);

    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(unsigned char));

    videoTimeBase=fmtCtx->streams[videoStreamIndex]->time_base;

    // 显示视频相关的参数信息（编码上下文）
    qDebug() << "比特率:" << codecCtx->bit_rate;
    qDebug() << "宽高:" << codecCtx->width << "x" << codecCtx->height;
    qDebug() << "格式:" << codecCtx->pix_fmt;
    qDebug() << "帧率分母:" << codecCtx->time_base.den;
    qDebug() << "帧率分子:" << codecCtx->time_base.num;

    //    qDebug() << "总时长:" << pAVStream->duration / 10000.0 << "s";
    qDebug() << "总时长:" << pAVStream->duration *av_q2d(pAVStream->time_base) << "s";
    qDebug() << "总帧数:" << pAVStream->nb_frames;
    double fps = pAVStream->nb_frames / (pAVStream->duration *av_q2d(pAVStream->time_base));
    qDebug() << "平均帧率:" << fps;
    double interval =1000/fps;
    qDebug() << "帧间隔:" << interval << "ms";

    return true;
}

void opengldecoder::stop()
{
    stop_flag=true;
}

void opengldecoder::seek(int position)
{
    if(videoStreamIndex!=-1)
        av_seek_frame(fmtCtx,videoStreamIndex,(fmtCtx->streams[videoStreamIndex]->duration)*position/100,AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);
}

void opengldecoder::setVpts(qint64 *ptr)
{
    this->pVpts=ptr;
}

void opengldecoder::setApts(qint64 *ptr)
{
    this->pApts=ptr;
}

void opengldecoder::setAudioTimeBase(AVRational base)
{
    this->audioTimeBase=base;
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

    double msInOneFps = 1000 / (fmtCtx->streams[videoStreamIndex]->avg_frame_rate.num/fmtCtx->streams[videoStreamIndex]->avg_frame_rate.den); // 每秒60帧，则1帧就是约16毫秒

    //    timeBeginPeriod(1);
    double dqFreq;		//计时器频率
    LARGE_INTEGER f;	//计时器频率
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart/1000;   //计时器频率/1000，这样时间除以频率的结果不再是秒，而是毫秒

    LARGE_INTEGER time_now;
    LARGE_INTEGER time_last;
    double time_diff=0;     //帧间渲染时间间隔，用于动态控制帧率
    double av_diff=0;   //音视频之间的时间差，用于动态同步
    int sleep_time = msInOneFps;     //实际睡眠时间，受time_diff和av_diff影响而动态调整

    time_last.QuadPart=0;

    while(av_read_frame(fmtCtx,pkt)>=0&&!stop_flag){
        if(pkt->stream_index==videoStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                int ret;
                int bytes=0;
                while((ret=avcodec_receive_frame(codecCtx,yuvFrame))>=0&&!stop_flag){
                    *pVpts=yuvFrame->pts;
//                    qDebug()<<"视频pts"<<yuvFrame->pts;
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

                    // 帧率动态控制算法
                    QueryPerformanceCounter(&time_now);
                    time_diff=(time_now.QuadPart-time_last.QuadPart)/dqFreq;
                    time_last = time_now;

                    // pts音视频同步
                    if (*pApts != -1) {
                        av_diff=1000*((*pApts)*av_q2d(audioTimeBase)-(*pVpts)*av_q2d(videoTimeBase));
                        if (av_diff > 20) sleep_time-=2;
                        if (av_diff <-20) sleep_time+=2;
                    }

                    // 帧率动态控制
                    if(time_diff-msInOneFps>2) sleep_time--;
                    else if(time_diff-msInOneFps<2) sleep_time++;

                    if (sleep_time> 0)
                        Sleep(sleep_time);
                    qDebug()<<"sleeptime:"<<sleep_time;
                    qDebug()<<"timediff:"<<time_diff;
                    qDebug()<<"avdiff:" <<av_diff;
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
