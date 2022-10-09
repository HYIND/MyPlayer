#include "multimediawidget.h"
//多媒体类

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

MultimediaWidget::MultimediaWidget(QWidget *parent)
    :QOpenGLWidget(parent)
{
    this->videodecoder=new opengldecoder();
    this->audiodecoder=new AudioDecoder(this);

    videodecoder->setVpts(&vpts);
    videodecoder->setApts(&apts);
    audiodecoder->setVpts(&vpts);
    audiodecoder->setApts(&apts);

    connect(videodecoder,&opengldecoder::sigFirst,[=](uchar* p,int w,int h){
        ptr=p;
        width=w;
        height=h;
    });
    connect(videodecoder,&opengldecoder::newFrame,[=](){
        update();
        //        updateGL();
    });
    connect(audiodecoder,&AudioDecoder::time_base,this,&MultimediaWidget::setAudioTimeBase);

    connect(this,&MultimediaWidget::video_seek,videodecoder,&opengldecoder::seek);
    connect(this,&MultimediaWidget::audio_seek,audiodecoder,&AudioDecoder::seek);

}

MultimediaWidget::~MultimediaWidget()
{
}

void MultimediaWidget::seturl(QString url)
{
    audiodecoder->seturl(url);
    videodecoder->seturl(url);
}

void MultimediaWidget::play(QString url)
{
    stop();
    seturl(url);
    audiodecoder->start();
    videodecoder->start();
}

void MultimediaWidget::stop(){
    //    if(audiodecoder->isRunning())
    //    {
    audiodecoder->stop();
    audiodecoder->requestInterruption();
    audiodecoder->quit();
    audiodecoder->wait(100);
    //    }

    if(videodecoder->isRunning()){
        videodecoder->stop();
        videodecoder->requestInterruption();
        videodecoder->quit();
        videodecoder->wait(100);
    }

    vpts=-1;
    apts=-1;
}

void MultimediaWidget::seek(int position)
{
    emit audio_seek(position);
    if(videodecoder->isRunning()){
        emit video_seek(position);
    }
}

void MultimediaWidget::setAudioTimeBase(AVRational base)
{
    videodecoder->setAudioTimeBase(base);
}

void MultimediaWidget::initializeGL()
{
    qDebug() << "initializeGL";

    //初始化opengl （QOpenGLFunctions继承）函数
    initializeOpenGLFunctions();

    //顶点shader
    const char *vString =
            "attribute vec4 vertexPosition;\
            attribute vec2 textureCoordinate;\
    varying vec2 texture_Out;\
    void main(void)\
    {\
        gl_Position = vertexPosition;\
        texture_Out = textureCoordinate;\
    }";
    //片元shader
    const char *tString =
            "varying vec2 texture_Out;\
            uniform sampler2D tex_y;\
    uniform sampler2D tex_u;\
    uniform sampler2D tex_v;\
    void main(void)\
    {\
        vec3 YUV;\
        vec3 RGB;\
        YUV.x = texture2D(tex_y, texture_Out).r;\
        YUV.y = texture2D(tex_u, texture_Out).r - 0.5;\
        YUV.z = texture2D(tex_v, texture_Out).r - 0.5;\
        RGB = mat3(1.0, 1.0, 1.0,\
                   0.0, -0.39465, 2.03211,\
                   1.13983, -0.58060, 0.0) * YUV;\
        gl_FragColor = vec4(RGB, 1.0);\
    }";

    //m_program加载shader（顶点和片元）脚本
    //片元（像素）
    qDebug()<<m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, tString);
    //顶点shader
    qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vString);

    //设置顶点位置
    m_program.bindAttributeLocation("vertexPosition",ATTRIB_VERTEX);
    //设置纹理位置
    m_program.bindAttributeLocation("textureCoordinate",ATTRIB_TEXTURE);

    //编译shader
    qDebug() << "m_program.link() = " << m_program.link();

    qDebug() << "m_program.bind() = " << m_program.bind();

    //传递顶点和纹理坐标
    //顶点
    static const GLfloat ver[] = {
        -1.0f,-1.0f,
        1.0f,-1.0f,
        -1.0f, 1.0f,
        1.0f,1.0f
        //        -1.0f,-1.0f,
        //        0.9f,-1.0f,
        //        -1.0f, 1.0f,
        //        0.9f,1.0f
    };
    //纹理
    static const GLfloat tex[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    //设置顶点,纹理数组并启用
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, ver);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, tex);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    //从shader获取地址
    m_textureUniformY = m_program.uniformLocation("tex_y");
    m_textureUniformU = m_program.uniformLocation("tex_u");
    m_textureUniformV = m_program.uniformLocation("tex_v");

    //创建纹理
    glGenTextures(1, &m_idy);
    //Y
    glBindTexture(GL_TEXTURE_2D, m_idy);
    //放大过滤，线性插值   GL_NEAREST(效率高，但马赛克严重)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //U
    glGenTextures(1, &m_idu);
    glBindTexture(GL_TEXTURE_2D, m_idu);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //V
    glGenTextures(1, &m_idv);
    glBindTexture(GL_TEXTURE_2D, m_idv);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void MultimediaWidget::resizeGL()
{
    if(height<=0) height=1;

    glViewport(0,0,width,height);
}

void MultimediaWidget::paintGL()
{
    if(ptr ==NULL) return;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idy);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,ptr);
    //与shader 关联
    glUniform1i(m_textureUniformY, 0);

    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, m_idu);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr+width*height);
    //与shader关联
    glUniform1i(m_textureUniformU,1);

    glActiveTexture(GL_TEXTURE0+2);
    glBindTexture(GL_TEXTURE_2D, m_idv);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr+width*height*5/4);
    //与shader 关联
    glUniform1i(m_textureUniformV, 2);

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    //    qDebug() << "paintGL";
}
