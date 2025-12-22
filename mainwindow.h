#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMovie>
#include <QLabel>
#include <QTimer>

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
    void animateIfVisible();
private:
    Ui::MainWindow *ui;
    void loadGifsFromFolder(const QString &path);
    bool isWidgetVisibleInViewport(QWidget *w);

    struct GifItem { //mby need dont use struct
        QLabel *label = nullptr;
        QMovie *movie = nullptr;
    };

    QMap<QWidget*, GifItem> items;
    QTimer *visibility_timer = nullptr;
    const QSize thumbnailSize = QSize(160, 120); //default GIFs size
    const int columns = 3; //default thubnail columns
};

#endif // MAINWINDOW_H
