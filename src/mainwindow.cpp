#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->horizontalSlider,&QSlider::valueChanged,ui->ffmpegwidget,MultimediaWidget::seek);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_play_Button_clicked()
{
//    ui->ffmpegwidget->seturl(ui->line_url->text());
    ui->ffmpegwidget->play(ui->line_url->text());
}


void MainWindow::on_stop_Button_clicked()
{
    ui->ffmpegwidget->stop();
}



void MainWindow::on_horizontalSlider_sliderReleased()
{
}

