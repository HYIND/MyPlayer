#include "audiodecoder.h"
//音频解码器

#define MAX_AUDIO_FRAME_SIZE 192000

AudioDecoder::AudioDecoder(QObject* parent)
{
    m_outputDevices = new QMediaDevices(parent);
    m_outputDevice = m_outputDevices->defaultAudioOutput();
    QAudioFormat format = m_outputDevice.preferredFormat();
    format.setSampleRate(48000);
    format.setSampleFormat(format.SampleFormat::Float);
    // ChannelConfigStereo is 2, Int16 is 2
    qDebug("sampleRate: %d, channelCount: %d, sampleFormat: %d",
           format.sampleRate(), format.channelCount(), format.sampleFormat()
           );
    m_audioSinkOutput = new QAudioSink(m_outputDevice, format);
    //    QThread* audioSinkThread=new QThread;
    //    m_audioSinkOutput->moveToThread(audioSinkThread);
    //    audioSinkThread->start();

    m_pcmInputDevice = new PcmInputDevice;
    QThread* pcmInThread=new QThread;
    m_pcmInputDevice->moveToThread(pcmInThread);
    pcmInThread->start();

    connect(this,&AudioDecoder::pcmIn_start,this->m_pcmInputDevice,&PcmInputDevice::mystart);
    connect(this,&AudioDecoder::pcmIn_stop,this->m_pcmInputDevice,&PcmInputDevice::mystop);
    connect(this,&AudioDecoder::audiosink_start,this->m_audioSinkOutput,QOverload<QIODevice*>::of(&QAudioSink::start));
    connect(this,&AudioDecoder::audiosink_stop,this->m_audioSinkOutput,&QAudioSink::stop);
    connect(this,&AudioDecoder::pcmIn_seek,this->m_pcmInputDevice,&PcmInputDevice::seek);

    connect(this->m_pcmInputDevice,&PcmInputDevice::time_base,this,&AudioDecoder::setAudioTimeBase);
}

AudioDecoder::~AudioDecoder()
{

}

void AudioDecoder::seturl(QString url)
{
    _url=url;
}

void AudioDecoder::seek(int position)
{
    emit pcmIn_seek(position);
}

//bool AudioDecoder::open_file()
//{
//    return m_pcmInputDevice->open_file(this->_url);
//}

void AudioDecoder::stop()
{
    emit pcmIn_stop();
    //    m_pcmInputDevice->stop();
    emit audiosink_stop();
    //    m_audioSinkOutput->stop();
}

void AudioDecoder::setVpts(qint64 *ptr)
{
    m_pcmInputDevice->setVpts(ptr);
}

void AudioDecoder::setApts(qint64 *ptr)
{
    m_pcmInputDevice->setApts(ptr);
}

void AudioDecoder::setAudioTimeBase(AVRational base)
{
    emit time_base(base);
}

void AudioDecoder::run(){
    emit pcmIn_start(this->_url);
    //    m_pcmInputDevice->start();
    emit audiosink_start(m_pcmInputDevice);
    //    m_audioSinkOutput->start(m_pcmInputDevice);
}

PcmInputDevice::PcmInputDevice()
{

}

bool PcmInputDevice::open_file()
{
    //    _url="D:\\MyPlayer\\test_video\\music2.mp4";
    if(_url.isEmpty())   return false;

    fmtCtx =avformat_alloc_context();
    pkt=av_packet_alloc();
    frame = av_frame_alloc();

    aStreamIndex = -1;

    if(avformat_open_input(&fmtCtx,_url.toStdString().c_str(),NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return false;
    }
    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return false;
    }

    av_dump_format(fmtCtx,0,_url.toStdString().c_str(),0);

    for(size_t i=0;i<fmtCtx->nb_streams;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            aStreamIndex=(int)i;
            break;
        }
    }
    if(aStreamIndex==-1){
        printf("Cannot find audio stream.\n");
        return false;
    }

    aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
    AVStream* pAVStream=fmtCtx->streams[aStreamIndex];
    codec = avcodec_find_decoder(aCodecPara->codec_id);
    if(!codec){
        printf("Cannot find any codec for audio.\n");
        return false;
    }
    codecCtx = avcodec_alloc_context3(codec);
    if(avcodec_parameters_to_context(codecCtx,aCodecPara)<0){
        printf("Cannot alloc codec context.\n");
        return false;
    }
    codecCtx->pkt_timebase=fmtCtx->streams[aStreamIndex]->time_base;

    if(avcodec_open2(codecCtx,codec,NULL)<0){
        printf("Cannot open audio codec.\n");
        return false;
    }

    //设置转码参数
    uint64_t out_channel_layout = codecCtx->channel_layout;
    out_sample_fmt = AV_SAMPLE_FMT_FLT;
    out_sample_rate = codecCtx->sample_rate;
    out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

    audio_out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

    swr_ctx = swr_alloc_set_opts(NULL,
                                 out_channel_layout,
                                 out_sample_fmt,
                                 out_sample_rate,
                                 codecCtx->channel_layout,
                                 codecCtx->sample_fmt,
                                 codecCtx->sample_rate,
                                 0,NULL);
    swr_init(swr_ctx);

    emit time_base(fmtCtx->streams[aStreamIndex]->time_base);

    // 显示视频相关的参数信息（编码上下文）
    qDebug() << "音频比特率:" << codecCtx->bit_rate;
    qDebug() << "音频宽高:" << codecCtx->width << "x" << codecCtx->height;
    qDebug() << "音频格式:" << codecCtx->pix_fmt;
    qDebug() << "音频帧率分母:" << codecCtx->time_base.den;
    qDebug() << "音频帧率分子:" << codecCtx->time_base.num;

    //    qDebug() << "音频帧率分母:" << pAVStream->avg_frame_rate.den;
    //    qDebug() << "音频帧率分子:" << pAVStream->avg_frame_rate.num;

    qDebug() << "音频帧率分母:" << pAVStream->time_base.den;
    qDebug() << "音频帧率分子:" << pAVStream->time_base.num;

    //    qDebug() << "总时长:" << pAVStream->duration / 10000.0 << "s";
    qDebug() << "音频总时长:" << pAVStream->duration *av_q2d(pAVStream->time_base) << "s";
    qDebug() << "音频总帧数:" << pAVStream->nb_frames;
    double fps = pAVStream->nb_frames / (pAVStream->duration *av_q2d(pAVStream->time_base));
    qDebug() << "音频平均帧率:" << fps;
    double interval =1000/fps;
    qDebug() << "音频帧间隔:" << interval << "ms";

    return true;
}

void PcmInputDevice::seek(int position)
{
    if(aStreamIndex!=-1)
        av_seek_frame(fmtCtx,aStreamIndex,(fmtCtx->streams[aStreamIndex]->duration)*position/100,AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);
}

void PcmInputDevice::setVpts(qint64 *ptr)
{
    this->pVpts=ptr;
}

void PcmInputDevice::setApts(qint64 *ptr)
{
    this->pApts=ptr;
}

void PcmInputDevice::start()
{
    if(!open_file()){
        printf("open_file failed!");
        return;
    }
    open(QIODevice::ReadOnly);
}

void PcmInputDevice::stop()
{
    close();
}

PcmInputDevice::~PcmInputDevice()
{
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    avformat_free_context(fmtCtx);

    //    m_inputFile.close();
}

/**
 * \brief 扬声器缺少数据时会自动调用这个方法
 */
qint64 PcmInputDevice::readData(char *data, qint64 len)
{
    qint64 goal_len=len/2;
    qint64 sum_len=0;
    while(av_read_frame(fmtCtx,pkt)>=0){
        if(pkt->stream_index==aStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                while(avcodec_receive_frame(codecCtx,frame)>=0){
                    /*
                          Planar（平面），其数据格式排列方式为 (特别记住，该处是以点nb_samples采样点来交错，不是以字节交错）:
                          LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...（每个LLLLLLRRRRRR为一个音频帧）
                          而不带P的数据格式（即交错排列）排列方式为：
                          LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）
                        */
//                    qDebug()<<"音频pts"<<frame->pts;
                    *pApts=frame->pts;

                    if(av_sample_fmt_is_planar(codecCtx->sample_fmt)){
                        int len = swr_convert(swr_ctx,
                                              //                                                  &audio_out_buffer,////////
                                              (uint8_t**)&(data),
                                              MAX_AUDIO_FRAME_SIZE*2,
                                              (const uint8_t**)frame->data,
                                              frame->nb_samples);
                        if(len<=0){
                            continue;
                        }
                        int out_size = av_samples_get_buffer_size(0,
                                                                  out_channels,
                                                                  len,
                                                                  out_sample_fmt,
                                                                  1);
                        sum_len +=out_size;
                        data+=out_size;
                        if(sum_len>goal_len)
                            return sum_len;
                        //int numBytes =av_get_bytes_per_sample(out_sample_fmt);
                        //printf("number bytes is: %d.\n",numBytes);

                        //                            fwrite(audio_out_buffer,1,out_size,file);

                        //                            sleep_time=(out_sample_rate*16*2/8)/out_size;

                        //                            if(audioOutput->bytesFree()<out_size){
                        //                            QTest::qSleep(sleep_time);
                        //                                (char*)audio_out_buffer,out_size;
                        //                            }else {
                        //                                (char*)audio_out_buffer,out_size;
                        //                            }

                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    return sum_len;
    //    m_inputFile.read(data, len);

    return 0;
}

qint64 PcmInputDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

void PcmInputDevice::mystop()
{
    this->stop();
}

void PcmInputDevice::mystart(QString url)
{
    this->_url=url;
    this->start();
}
