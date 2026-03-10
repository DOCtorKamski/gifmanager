#include "gif_viewer.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>

GifViewer::GifViewer(const QString &file_path, QWidget *parent)
    : QWidget(parent), file_path(file_path)
{
    setWindowTitle(QFileInfo(file_path).fileName());
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    image_label = new QLabel();
    image_label->setAlignment(Qt::AlignCenter);
    image_label->setStyleSheet("background: #000;");

    movie = new QMovie(file_path);
    movie->setCacheMode(QMovie::CacheAll);
    image_label->setMovie(movie);
    movie->start();

    layout->addWidget(image_label);
    setLayout(layout);
    resize(800, 600);
}

GifViewer::~GifViewer() {
    if (movie) {
        movie->stop();
        movie->deleteLater();
    }
}

void GifViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        close();
    }
    else if (event->button() == Qt::RightButton) {
        contextMenuEvent(new QContextMenuEvent(QContextMenuEvent::Mouse, event->pos()));
    }
    QWidget::mousePressEvent(event);
}

void GifViewer::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *rename_act = menu.addAction(tr("Rename"));

    QAction *act = menu.exec(mapToGlobal(event->pos()));
    if (act == rename_act) {
        emit renameRequested(file_path);
    }
}
