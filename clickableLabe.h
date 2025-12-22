#ifndef CLICKABLELABE_H
#define CLICKABLELABE_H

#include <QLabel>
#include <QContextMenuEvent>
#include <QMenu>

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}
    void setFilePath(const QString &p) { filePath = p; }
protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu menu(this);
        QAction *copy_act = menu.addAction(tr("Copy GIF"));
        QAction *act = menu.exec(event->globalPos());
        if (act == copy_act) emit copyRequested(filePath);
    }
signals:
    void copyRequested(const QString &filePath);
private:
    QString filePath;
};

#endif // CLICKABLELABE_H
