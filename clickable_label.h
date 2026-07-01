#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QContextMenuEvent>
#include <QMenu>

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr);
    QString filePath() const { return file_path; }
    void setFilePath(const QString &new_file_path);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void copyRequested(const QString &file_path);
    void deleteRequested(const QString &file_path);
    void openFullSizeRequested(const QString &file_path);
    void renameRequested(const QString &file_path);

private:
    QString file_path;
};

#endif // CLICKABLELABEL_H
