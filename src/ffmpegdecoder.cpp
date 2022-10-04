#include "ffmpegdecoder.h"
#include <QDateTime>
//ffmpeg解码器，待用

static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef* hw_device_ctx=NULL;


FFmpegdecoder::FFmpegdecoder()
{
    this->fmtCtx=avformat_alloc_context();
    this->pkt=av_packet_alloc();
    this->yuvFrame=av_frame_alloc();
    this->nv12Frame=av_frame_alloc();
    this->rgbFrame=av_frame_alloc();
}

FFmpegdecoder::~FFmpegdecoder()
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

void FFmpegdecoder::seturl(QString url)
{
    _url=url;
}

int FFmpegdecoder::hw_decoder_init(AVCodecContext *ctx, const AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

AVPixelFormat get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
{
    Q_UNUSED(ctx)
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

int FFmpegdecoder::open_file()
{
    if(_url.isEmpty()) return -1;

    enum AVHWDeviceType type;
    int i;

    type = av_hwdevice_find_type_by_name("dxva2");
    if (type == AV_HWDEVICE_TYPE_NONE) {
        fprintf(stderr, "Device type %s is not supported.\n", "h264_cuvid");
        fprintf(stderr, "Available device types:");
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
        fprintf(stderr, "\n");
        return -1;
    }

    if(avformat_open_input(&fmtCtx,_url.toStdString().c_str(),NULL,NULL)<0) return -1;

    if(avformat_find_stream_info(fmtCtx,NULL)<0) return -1;

    //    int streamCnt=fmtCtx->nb_streams;
    //    for (int i=0; i<streamCnt;i++){
    //        if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
    //            videoStreamIndex=i;
    //            continue;
    //        }
    //    }

    //    if(videoStreamIndex==-1) return -1;

    //    AVCodecParameters* codecPara =fmtCtx->streams[videoStreamIndex]->codecpar;

    //    if(!(codec=avcodec_find_decoder(codecPara->codec_id))) return -1;


    if (videoStreamIndex=av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0) < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }

    //获取支持该decoder的hw配置型
    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    codec->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }


    if(!(codecCtx=avcodec_alloc_context3(codec))) return -1;

    if(avcodec_parameters_to_context(codecCtx,fmtCtx->streams[videoStreamIndex]->codecpar)) return -1;

    codecCtx->get_format=get_hw_format;

    if(hw_decoder_init(codecCtx,type)<0)return -1;

    if(avcodec_open2(codecCtx,codec,NULL)<0)return -1;

    img_ctx=sws_getContext(codecCtx->width,
                           codecCtx->height,
                           AV_PIX_FMT_NV12,
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

void FFmpegdecoder::stop()
{
    stop_flag=true;
}

void FFmpegdecoder::run()
{
    stop_flag=false;
    if(open_file()==-1 )return;

    int ms_perframe=1000/(fmtCtx->streams[videoStreamIndex]->avg_frame_rate.num/fmtCtx->streams[videoStreamIndex]->avg_frame_rate.den);

    qint64 goal=QDateTime::currentDateTime().toMSecsSinceEpoch()+ms_perframe;
    qint64 now=0;

    while(av_read_frame(fmtCtx,pkt)>=0&&!stop_flag){
        if(pkt->stream_index==videoStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                int ret;
                while((ret=avcodec_receive_frame(codecCtx,yuvFrame))>=0&&!stop_flag){
                    now=QDateTime::currentDateTime().toMSecsSinceEpoch();
                    if(now<goal) QThread::msleep(goal-now);
                    goal+=ms_perframe;

                    if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF) return;
                    else if(ret<0) return;

                    if(yuvFrame->format==codecCtx->pix_fmt){
                        if(av_hwframe_transfer_data(nv12Frame,yuvFrame,0)<0){
                            continue;
                        }
                    }

                    sws_scale(img_ctx,
                              nv12Frame->data,nv12Frame->linesize,
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
