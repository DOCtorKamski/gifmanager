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
    QStringList gif_files = {
        "/path/to/my_file1.gif"
        "/path/to/my_file2.gif"
        "/path/to/my_file3.gif"
        "/path/to/my_file4.gif"
        "/path/to/my_file5.gif"
    };

    //TODO move this
    int row_count{};
    int col_count{};

    for(const QString &gif_path : gif_files) {
        QLabel *label = new QLabel();
        QMovie *movie = new QMovie(gif_path);
        label->setMovie(movie);
        movie->start();

        //resize label to gif size TODO make like method
        QSize gif_size = movie->currentImage().size();
        label->setFixedSize(gif_size);

        // add label to grid layout
        ui->gridLayout->addWidget(label, row_count, col_count);

        // update row and column index
        col_count++;
        if (col_count >= 3) { // TODO add to change to desired number of columns
            col_count = 0;
            row_count++;
        }
    }
}

MainWindow::~MainWindow()
{
    delete movie;
    delete ui;
}

