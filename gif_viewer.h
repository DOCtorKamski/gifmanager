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

private:
    QLabel *image_label;
    QMovie *movie;
};

#endif // GIF_VIEWER_H
