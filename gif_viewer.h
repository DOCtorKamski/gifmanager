#ifndef GIF_VIEWER_H
#define GIF_VIEWER_H

#include <QLabel>
#include <QMovie>
#include <QSize>
#include <QWidget>

class GifViewer : public QWidget {
    Q_OBJECT
public:
    explicit GifViewer(const QString &file_path, QSize viewer_size = QSize(800, 600),
                       const QString &bg_style = "background: #000;",
                       const QString &label_style = "background: #333; color: #fff; padding: 4px;",
                       QWidget *parent = nullptr);
    ~GifViewer();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void copyRequested(const QString &file_path);
    void deleteRequested(const QString &file_path);
    void renameRequested(const QString &file_path);

private:
    QLabel *image_label = nullptr;
    QLabel *file_name_label = nullptr;
    QMovie *movie = nullptr;
    QString file_path;
    QSize viewer_size;
    QString bg_style;
    QString label_style;
};

#endif // GIF_VIEWER_H
