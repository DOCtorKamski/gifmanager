#include "clickable_label.h"

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent) {}

void ClickableLabel::setFilePath(const QString &p) {
    file_path = p;
}

void ClickableLabel::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *copy_act = menu.addAction(tr("Copy GIF"));
    QAction *act = menu.exec(event->globalPos());
    if (act == copy_act) {
        emit copyRequested(file_path);
    }
}
