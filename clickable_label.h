#ifndef CLICKABLELABE_H
#define CLICKABLELABE_H

#include <QLabel>
#include <QContextMenuEvent>
#include <QMenu>

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr);
    void setFilePath(const QString &p);
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
signals:
    void copyRequested(const QString &filePath);
private:
    QString file_path;
};

#endif // CLICKABLELABE_H
