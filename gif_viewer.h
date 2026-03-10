#ifndef GIF_VIEWER_H
#define GIF_VIEWER_H

#include <QWidget>
#include <QLabel>
#include <QMovie>

class GifViewer : public QWidget {
    Q_OBJECT
public:
    explicit GifViewer(const QString &file_path, QWidget *parent = nullptr);
    ~GifViewer();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void renameRequested(const QString &file_path);

private:
    QLabel *image_label;
    QMovie *movie;
    QString file_path;
};

#endif // GIF_VIEWER_H
