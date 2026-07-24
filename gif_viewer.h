#ifndef GIF_VIEWER_H
#define GIF_VIEWER_H

#include <QLabel>
#include <QMovie>
#include <QWidget>

class GifViewer : public QWidget {
    Q_OBJECT
public:
    explicit GifViewer(const QString &file_path, QWidget *parent = nullptr);
    ~GifViewer();

    QString filePath() const { return file_path; }

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
};

#endif // GIF_VIEWER_H
