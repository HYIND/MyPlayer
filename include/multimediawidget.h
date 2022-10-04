#ifndef MULTIMEDIAWIDGET_H
#define MULTIMEDIAWIDGET_H

//多媒体播放类，QOpenglwidget，包括ffmpeg视频解码器(opengldecoder.h)和音频解码器(audiodecoder.h)

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLTexture>

#include "opengldecoder.h"
#include "audiodecoder.h"

class MultimediaWidget :public QOpenGLWidget,public QOpenGLFunctions{
    Q_OBJECT
public:
    explicit MultimediaWidget(QWidget *parent = nullptr);
    ~MultimediaWidget();

    void seturl(QString url);

    void play(QString url);
    void stop();

    void initializeGL();
    void resizeGL();
    void paintGL();

private:
    QOpenGLShaderProgram m_program;     //shader程序
    GLuint m_textureUniformY,m_textureUniformU,m_textureUniformV;   //shader中yuv变量地址
    GLuint m_idy,m_idu,m_idv;   //创建纹理

    int width,height;

    opengldecoder* videodecoder;
    AudioDecoder* audiodecoder;
    uchar* ptr;
};

#endif // MULTIMEDIAWIDGET_H
