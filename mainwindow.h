#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clickable_label.h"
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
    void copyGifToClipboard (const QString &file_path);
    void animateIfVisible();
    void openFullSizeGif(const QString &file_path);
    void renameGif(const QString &file_path);

private:
    Ui::MainWindow *ui;
    void clearItems();
    void loadGifsFromFolder(const QString &folder_path);
    bool isWidgetVisibleInViewport(QWidget *w);

    struct GifItem { //mby need dont use struct TODO
        ClickableLabel *label = nullptr;
        QMovie *movie = nullptr;
    };

    QMap<QWidget*, GifItem> items; //TODO need better name
    QString current_folder;
    QStringList all_gif_files; // full paths of all GIFs in folder
    QTimer *search_timer = nullptr;
    QTimer *visibility_timer = nullptr;
    const QSize thumbnailSize = QSize(160, 120); //default GIFs size
    const int columns = 3; //default thubnail columns
};

#endif // MAINWINDOW_H
