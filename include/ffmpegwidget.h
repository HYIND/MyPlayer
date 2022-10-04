#ifndef FFMPEGWIDGET_H
#define FFMPEGWIDGET_H

//ffmpeg播放视频类，待用

#include "ffmpegdecoder.h"
#include "header.h"
#include <QWidget>
#include <QPainter>


class FFmpegwidget : public QWidget
{
    Q_OBJECT
public:
    explicit FFmpegwidget(QWidget *parent = nullptr);
    ~FFmpegwidget();

    void seturl(QString url);

    void play(QString url);
    void stop();

protected:
    void paintEvent(QPaintEvent* event);

private slots:
    void recviveQImage(const QImage& rImg);

private:

    FFmpegdecoder* ffmpeg;
    QImage img;
};

#endif // FFMPEGWIDGET_H
