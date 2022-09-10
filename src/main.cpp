#include "mainwindow.h"
#include <iostream>
#include<fstream>
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

#if 0
void saveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int y;

    // 打开文件
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    // 写入文件头
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // 写入像素数据
    for (y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

    // 关闭文件
    fclose(pFile);
}
#endif


int main(int argc, char *argv[])
{
    //    setbuf(stdout, NULL);
    QApplication a(argc, argv);

#if 0
    //    av_register_all();
    unsigned codecVer = avcodec_version();
    int ver_major,ver_minor,ver_micro;
    ver_major = (codecVer>>16)&0xff;
    ver_minor = (codecVer>>8)&0xff;
    ver_micro = (codecVer)&0xff;
    qDebug()<<"avcodec version is:"<<codecVer<<ver_major<<'.'<<ver_minor<<'.'<<ver_micro;
#endif


#if 0
    AVFormatContext *fmt_ctx = avformat_alloc_context();//创建对象并初始化
    int ret=0;
    char* fileName= (char*)"D:/MyPlayer/test_video/testvideo2.mp4";//文件地址
    //    char* fileName= (char*)"testvideo1.mp4";//文件地址
    //        QString fileName = "D:/MyPlayer/test_video/testvideo1.mp4";

    do{
        //打开文件
        if ((ret = avformat_open_input(&fmt_ctx, (char*)fileName, NULL, NULL))<0){
            qDebug()<<"open file failed!";
            break;//Cannot open video file
        }

        //查找流信息（音频流和视频流）
        if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
            qDebug()<<"Cannot find stream information\n";
            break;
        }

        av_dump_format(fmt_ctx,0, (char*)fileName,0);//输出视频信息
        //fflush(stdout);
    }while(0);

    avformat_close_input(&fmt_ctx);//关闭文件
    return ret;
#endif

#if 0
    FILE* fp=fopen("result_nv12.yuv","w+b");
    if(!fp){
        printf("cannot open file!");
        return -1;
    }

    char filePath[]="D:/MyPlayer/test_video/testvideo2.mp4";    //视频地址
    int videoStreamIndex = -1;  //视频流所在流序列中的索引
    int ret=0;  //默认返回值

    // 需要的变量名并初始化
    AVFormatContext *fmtCtx=NULL;
    AVPacket *pkt=NULL;
    AVCodecContext *codecCtx=NULL;
    AVCodecParameters *avCodecpara=NULL;
    const AVCodec *codec=NULL;

    AVFrame *yuvFrame = av_frame_alloc();
    AVFrame *nv12Frame = av_frame_alloc();

    do{
        //=========================== 创建AVFormatContext结构体 ===============================//
        //分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
        fmtCtx =avformat_alloc_context();

        //=========================== 打开文件 ===============================//
        ret=avformat_open_input(&fmtCtx,filePath,NULL,NULL);
        if(ret!=0){
            printf("cannot open video file\n");
            break;
        }

        //=========================== 获取视频流信息 ===============================//
        ret=avformat_find_stream_info(fmtCtx,NULL);
        if(ret<0){
            printf("cannot retrive video info\n");
            break;
        }

        //循环查找视频中包含的流信息，直到找到视频类型的流
        //便将其记录下来，保存到viedoStreamindex中
        for(unsigned int i=0;i<fmtCtx->nb_streams;i++){
            if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
                videoStreamIndex=i;
                break;
            }
        }

        //如果videoStream为-1,则没有找到视频流
        if(videoStreamIndex==-1){
            printf("cannot find video stream\n");
            break;
        }

        //打印输入和输出信息，长度、比特率、流格式等
        av_dump_format(fmtCtx,0,filePath,0);

        //=========================== 查找编码器 ===============================//
        avCodecpara=fmtCtx->streams[videoStreamIndex]->codecpar;
        codec=avcodec_find_decoder(avCodecpara->codec_id);
        if(codec==NULL){
            printf("cannot find decoder\n");
            break;
        }

        //根据解码器参数来创建解码器内容
        codecCtx=avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx,avCodecpara);
        if(codecCtx==NULL){
            printf("Cannot allo context");
            break;
        }

        //=========================== 打开解码器 ===============================//
        ret=avcodec_open2(codecCtx,codec,NULL);
        if(ret<0){
            printf("cannot open deoder\n");
            break;
        }

        //=========================== 设置数据转换参数 ===============================//
        SwsContext* img_ctx=sws_getContext(codecCtx->width,codecCtx->height,AV_PIX_FMT_YUV420P,
                                           codecCtx->width,codecCtx->height,AV_PIX_FMT_NV12,
                                           SWS_BICUBIC,NULL,NULL,NULL);

        //==================================== 分配空间 ==================================//
        int numBytes=av_image_get_buffer_size(AV_PIX_FMT_NV12, codecCtx->width, codecCtx->height, 1);
        unsigned char *out_buffer = (unsigned char *)av_malloc(numBytes * sizeof(unsigned char));

        //=========================== 分配AVPacket结构体 ===============================//
        int count=0;    //帧计数
        pkt=av_packet_alloc();  //分配一个packet
        av_new_packet(pkt,codecCtx->width*codecCtx->height);    //调整packet的数据

        // 会将pFreamRGB的数据按RGB格式自动"关联"到buffer
        // pFreamRGB的数据改变了，out_buffer中的数据也会相应改变
        av_image_fill_arrays(nv12Frame->data,nv12Frame->linesize,out_buffer,AV_PIX_FMT_NV12,
                             codecCtx->width,codecCtx->height,1);

        //=========================== 读取视频信息 ===============================//
        while(av_read_frame(fmtCtx,pkt)>=0){    //读取的是一帧视频，数据存入AVPacket结构
            if(pkt->stream_index==videoStreamIndex){
                count++;    //只统计视频帧
                if(avcodec_send_packet(codecCtx,pkt)==0){
                    while (avcodec_receive_frame(codecCtx, yuvFrame) == 0){
                        sws_scale(img_ctx,
                                  yuvFrame->data,
                                  yuvFrame->linesize,
                                  0,
                                  codecCtx->height,
                                  nv12Frame->data,
                                  nv12Frame->linesize);
                        fwrite(nv12Frame->data[0],1,codecCtx->width*codecCtx->height,fp);
                        fwrite(nv12Frame->data[1],1,codecCtx->width*codecCtx->height/2,fp);
                    }
                }
            }
            av_packet_unref(pkt);   //重置pkt内容
        }
        printf("There are %d frames int total.\n",count);
    }while(0);

    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    av_frame_free(&yuvFrame);
    av_frame_free(&nv12Frame);

    return ret;

#endif

#if 0
    FILE *fp=fopen("result.yuv","w+b");
    if(fp==NULL){
        printf("Cannot open file.\n");
        return -1;
    }
    //    ofstream ofs("result.yuv", ofstream::app);
    //    if(!ofs.is_open()){
    //        printf("Cannot open file.\n");
    //        return -1;
    //    }

    char filePath[]       = "D:/MyPlayer/test_video/testvideo2.mp4";//文件地址
    int  videoStreamIndex = -1;//视频流所在流序列中的索引
    int ret=0;//默认返回值

    //需要的变量名并初始化
    AVFormatContext *fmtCtx=NULL;
    AVPacket *pkt =NULL;
    AVCodecContext *codecCtx=NULL;
    AVCodecParameters *avCodecPara=NULL;
    const AVCodec *codec=NULL;
    AVFrame *yuvFrame = av_frame_alloc();

    unsigned char *out_buffer;

    do{
        //=========================== 创建AVFormatContext结构体 ===============================//
        //分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
        fmtCtx = avformat_alloc_context();
        //==================================== 打开文件 ======================================//
        if ((ret=avformat_open_input(&fmtCtx, filePath, NULL, NULL)) != 0) {
            printf("cannot open video file\n");
            break;
        }

        //=================================== 获取视频流信息 ===================================//
        if ((ret=avformat_find_stream_info(fmtCtx, NULL)) < 0) {
            printf("cannot retrive video info\n");
            break;
        }

        //循环查找视频中包含的流信息，直到找到视频类型的流
        //便将其记录下来 保存到videoStreamIndex变量中
        for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
            if (fmtCtx->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;//找到视频流就退出
            }
        }

        //如果videoStream为-1 说明没有找到视频流
        if (videoStreamIndex == -1) {
            printf("cannot find video stream\n");
            break;
        }

        //打印输入和输出信息：长度 比特率 流格式等
        av_dump_format(fmtCtx, 0, filePath, 0);

        //=================================  查找解码器 ===================================//
        avCodecPara = fmtCtx->streams[ videoStreamIndex ]->codecpar;
        codec       = avcodec_find_decoder(avCodecPara->codec_id);
        if (codec == NULL) {
            printf("cannot find decoder\n");
            break;
        }
        //根据解码器参数来创建解码器内容
        codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx, avCodecPara);
        if (codecCtx == NULL) {
            printf("Cannot alloc context.");
            break;
        }

        //================================  打开解码器 ===================================//
        if ((ret=avcodec_open2(codecCtx, codec, NULL)) < 0) { // 具体采用什么解码器ffmpeg经过封装 我们无须知道
            printf("cannot open decoder\n");
            break;
        }

        int w=codecCtx->width;//视频宽度
        int h=codecCtx->height;//视频高度

        //=========================== 分配AVPacket结构体 ===============================//
        pkt = av_packet_alloc();                      //分配一个packet
        av_new_packet(pkt, codecCtx->width * codecCtx->height); //调整packet的数据

        //===========================  读取视频信息 ===============================//
        while (av_read_frame(fmtCtx, pkt) >= 0) { //读取的是一帧视频  数据存入一个AVPacket的结构中
            if (pkt->stream_index == videoStreamIndex){
                if (avcodec_send_packet(codecCtx, pkt) == 0){
                    while (avcodec_receive_frame(codecCtx, yuvFrame) == 0){
                        fwrite(yuvFrame->data[0],1,w*h,fp);//y
                        fwrite(yuvFrame->data[1],1,w*h/4,fp);//u
                        fwrite(yuvFrame->data[2],1,w*h/4,fp);//v
                    }
                }
            }
            av_packet_unref(pkt);//重置pkt的内容
        }
    }while(0);
    //===========================释放所有指针===============================//
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    av_frame_free(&yuvFrame);

    return ret;

#endif

    MainWindow w;
    w.show();
    w.setWindowTitle("MyPlayer");
    return a.exec();
}
