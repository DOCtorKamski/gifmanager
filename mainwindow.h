#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMovie>
//for ClickableLabel
#include <QLabel>
#include <QContextMenuEvent>
#include <QMenu>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void chooseFolder();
    void copyGifToClipboard (const QString &filePath);
private:
    void loadGifsFromFolder(const QString &path);
    Ui::MainWindow *ui;

    struct GifItem { //mby need dont use struct
        QLabel *label = nullptr;
        QMovie *movie = nullptr;
    };

    QMap<QWidget*, GifItem> items;
    const QSize thumbnailSize = QSize(160, 120); //default GIFs size
    const int columns = 3; //default thubnail columns
};

//move this on new .h file. Now just put here with logic lol
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}
    void setFilePath(const QString &p) { filePath = p; }
protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu menu(this);
        QAction *copyAct = menu.addAction(tr("Copy GIF"));
        QAction *act = menu.exec(event->globalPos());
        if (act == copyAct) emit copyRequested(filePath);
    }
signals:
    void copyRequested(const QString &filePath);
private:
    QString filePath;
};

#endif // MAINWINDOW_H
