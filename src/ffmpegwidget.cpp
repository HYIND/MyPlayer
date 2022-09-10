#include "ffmpegwidget.h"

FFmpegwidget::FFmpegwidget(QWidget *parent)
    : QWidget{parent}
{
    this->ffmpeg=new FFmpegVideo();
    connect(ffmpeg,SIGNAL(sendQImage(QImage)),this,SLOT(recviveQImage(QImage)));
    //    connect(ffmpeg,&FFmpegVideo::finished,ffmpeg,&FFmpegVideo::deleteLater);
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


FFmpegVideo::FFmpegVideo()
{
    this->fmtCtx=avformat_alloc_context();
    this->pkt=av_packet_alloc();
    this->yuvFrame=av_frame_alloc();
    this->rgbFrame=av_frame_alloc();

}

FFmpegVideo::~FFmpegVideo()
{
    if(pkt) av_packet_free(&pkt);
    if(yuvFrame) av_frame_free(&yuvFrame);
    if(rgbFrame) av_frame_free(&rgbFrame);
    if(codecCtx) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
    }
    if(fmtCtx) avformat_free_context(fmtCtx);
}

void FFmpegVideo::seturl(QString url)
{
    _url=url;
}

int FFmpegVideo::open_file()
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

    img_ctx=sws_getContext(codecCtx->width,
                           codecCtx->height,
                           codecCtx->pix_fmt,
                           codecCtx->width,
                           codecCtx->height,
                           AV_PIX_FMT_RGB32,
                           SWS_BICUBIC,NULL,NULL,NULL);

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32,codecCtx->width,codecCtx->height,1);

    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(unsigned char));

    if(av_image_fill_arrays(
                rgbFrame->data,rgbFrame->linesize,
                out_buffer,AV_PIX_FMT_RGB32,
                codecCtx->width,codecCtx->height,1)<0)
        return -1;

    return true;
}

void FFmpegVideo::stop()
{
    stop_flag=true;
}

void FFmpegVideo::run()
{
    stop_flag=false;
    if(open_file()==-1 )return;

    while(av_read_frame(fmtCtx,pkt)>=0&&!stop_flag){
        if(pkt->stream_index==videoStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                int ret;
                while((ret=avcodec_receive_frame(codecCtx,yuvFrame))>=0&&!stop_flag){
                    if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF) return;
                    else if(ret<0) return;

                    sws_scale(img_ctx,
                              yuvFrame->data,yuvFrame->linesize,
                              0,codecCtx->height,
                              rgbFrame->data,rgbFrame->linesize);

                    QImage img(out_buffer,
                               codecCtx->width,codecCtx->height,
                               QImage::Format_RGB32);

                    emit sendQImage(img);
                    //                    QThread::msleep(30);
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
