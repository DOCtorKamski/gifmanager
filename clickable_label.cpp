#include "clickable_label.h"

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent) {}

void ClickableLabel::setFilePath(const QString &p) {
    file_path = p;
}

void ClickableLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit openFullSizeRequested(file_path);
    }
    QLabel::mousePressEvent(event);
}

void ClickableLabel::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *copy_act = menu.addAction(tr("Copy GIF"));
    QAction *rename_act = menu.addAction(tr("Rename"));

    QAction *act = menu.exec(event->globalPos());
    if (act == copy_act) {
        emit copyRequested(file_path);
    }
    else if (act == rename_act) {
        emit renameRequested(file_path);
    }
}
