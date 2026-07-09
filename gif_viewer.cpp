#include "gif_viewer.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QShortcut>

GifViewer::GifViewer(const QString &file_path, QWidget *parent)
    : QWidget(parent), file_path(file_path)
{
    setWindowTitle(QFileInfo(file_path).fileName());
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    image_label = new QLabel(this);
    image_label->setAlignment(Qt::AlignCenter);
    image_label->setStyleSheet(SettingsManager::instance().getViewerBgStyle());

    movie = new QMovie(file_path, {}, this);
    movie->setCacheMode(QMovie::CacheAll);
    if (!movie->isValid()) {
        delete movie;
        movie = nullptr;
        image_label->setText(tr("Failed to load GIF"));
    } else {
        image_label->setMovie(movie);
        movie->start();
    }

    file_name_label = new QLabel(QFileInfo(file_path).fileName(), this);
    file_name_label->setAlignment(Qt::AlignCenter);
    file_name_label->setStyleSheet(SettingsManager::instance().getViewerLabelStyle());
    layout->addWidget(file_name_label, 0);

    layout->addWidget(image_label, 1);

    setLayout(layout);
    resize(SettingsManager::instance().getViewerSize());
}

GifViewer::~GifViewer() {

}

void GifViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        close();
    }
    QWidget::mousePressEvent(event);
}

void GifViewer::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *copy_act = menu.addAction(tr("Copy GIF"));
    QAction *delete_act = menu.addAction(tr("Delete"));
    QAction *rename_act = menu.addAction(tr("Rename"));

    QAction *act = menu.exec(mapToGlobal(event->pos()));
    if (act == copy_act) {
        emit copyRequested(file_path);
    }
    else if (act == delete_act) {
        if (movie) {
            movie->stop();
            delete movie;
            movie = nullptr;
        }
        close();
        emit deleteRequested(file_path);
    }
    else if (act == rename_act) {
        if (movie) {
            movie->stop();
            delete movie;
            movie = nullptr;
        }
        close();
        emit renameRequested(file_path);
    }
}
