QT       += core gui  \
            openglwidgets \
            multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 \
       += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/audiodecoder.cpp \
    src/ffmpegdecoder.cpp \
    src/main.cpp  \
    src/mainwindow.cpp \
    src/ffmpegwidget.cpp \
    src/multimediawidget.cpp \
    src/opengldecoder.cpp \
    src/openglwidget2.cpp

HEADERS += \
    include/audiodecoder.h \
    include/ffmpegdecoder.h \
    include/ffmpegwidget.h \
    include/header.h \
    include/mainwindow.h \
    include/multimediawidget.h \
    include/opengldecoder.h \
    include/openglwidget2.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/ffmpeg/include \
    $$PWD/sdl/include

LIBS += \
    -L$$PWD/ffmpeg/lib -lavcodec \
                       -lavdevice \
                       -lavfilter \
                       -lavformat \
                       -lavutil \
                       -lpostproc \
                       -lswresample \
                       -lswscale  \
    -L$$PWD/sdl/lib -lSDL2 -lSDL2main  \


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
