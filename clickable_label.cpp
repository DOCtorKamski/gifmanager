#include "clickable_label.h"

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent) {}

void ClickableLabel::setFilePath(const QString &new_file_path) {
    file_path = new_file_path;
}

void ClickableLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit openFullSizeRequested(file_path);
    }
    //contextMenuEvent triggered automatically by Qt when the user right-clicks the widget
    QLabel::mousePressEvent(event);
}

void ClickableLabel::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *copy_act = menu.addAction(tr("Copy GIF"));
    QAction *delete_act = menu.addAction(tr("Delete"));
    QAction *rename_act = menu.addAction(tr("Rename"));

    QAction *act = menu.exec(event->globalPos());
    if (act == copy_act) {
        emit copyRequested(file_path);
    }
    else if (act == delete_act) {
        emit deleteRequested(file_path);
    }
    else if (act == rename_act) {
        emit renameRequested(file_path);
    }
}
