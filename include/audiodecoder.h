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
    void seek(int position);

    void stop();

    void setVpts(qint64 *ptr);
    void setApts(qint64 *ptr);
    void setAudioTimeBase(AVRational base);

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
    void pcmIn_seek(int position);
    void time_base(AVRational base);
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
    void seek(int position);

    void setVpts(qint64 *ptr);
    void setApts(qint64 *ptr);

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

signals:
    void time_base(AVRational base);

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

    qint64* pVpts;
    qint64* pApts;
};

#endif // AUDIODECODER_H
