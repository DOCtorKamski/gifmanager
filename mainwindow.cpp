#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //TODO change how find path
    movie = new QMovie("/path/to/my_file.gif");
    ui->label->setMovie(movie);
    ui->label->show();

    movie->start();

    //resize label to gif size TODO make like method
    QSize gif_size = movie->currentImage().size();
    ui->label->setFixedSize(gif_size);
}

MainWindow::~MainWindow()
{
    delete movie;
    delete ui;
}

