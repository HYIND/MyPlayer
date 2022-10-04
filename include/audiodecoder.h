#ifndef AUDIODECODER_H
#define AUDIODECODER_H

//音频解码器

#include "header.h"

#include <QThread>
#include <QIODevice>
#include <QDebug>
#include <QMediaDevices>
#include <QAudioSink>
#include <QAudioFormat>

class PcmInputDevice;

class AudioDecoder :public QThread{
    Q_OBJECT

public:
    AudioDecoder(QObject *parent);
    ~AudioDecoder();
    void seturl(QString url);
    //    bool open_file();
    void stop();

protected:
    void run();

private:
    QString _url;
    QAudioSink *m_audioSinkOutput;
    QMediaDevices *m_outputDevices;
    QAudioDevice m_outputDevice;
    PcmInputDevice *m_pcmInputDevice;

signals:
    void pcmIn_start(QString url);
    void pcmIn_stop();
    void audiosink_start(QIODevice *device);
    void audiosink_stop();
};

class PcmInputDevice : public QIODevice
{
    Q_OBJECT

public:
    PcmInputDevice();
    ~PcmInputDevice();
    void start();
    void stop();
    bool open_file();
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

public slots:
    void mystop();
    void mystart(QString url);

private:
    AVFormatContext *fmtCtx=NULL;
    AVCodecContext *codecCtx = NULL;
    AVPacket *pkt=NULL;
    AVFrame *frame=NULL;

    int aStreamIndex=-1;

    AVCodecParameters *aCodecPara=NULL;
    const AVCodec *codec =NULL;
    enum AVSampleFormat  out_sample_fmt = AV_SAMPLE_FMT_S16;

    int out_sample_rate;
    int out_channels;
    uint8_t *audio_out_buffer;

    SwrContext *swr_ctx;
    double sleep_time=0;

    QString _url;
};

#endif // AUDIODECODER_H
