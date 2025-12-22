#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMovie>
#include <QLabel>

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

#endif // MAINWINDOW_H
