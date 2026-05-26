#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clickable_label.h"
#include "gif_loader_worker.h"
#include <QMainWindow>
#include <QMovie>
#include <QLabel>
#include <QTimer>
#include <QMutex>

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
    void copyGifToClipboard(const QString &file_path);
    void animateIfVisible();
    void openFullSizeGif(const QString &file_path);
    void deleteGif(const QString &file_path);
    void renameGif(const QString &file_path);
    void onWorkerFinished();  // ← НОВОЕ

private:
    Ui::MainWindow *ui;
    void onGifLoaded(quint64 load_id, const LoadedGifData &data);
    void onLoadingFinished(quint64 load_id);
    void safeStopLoading();
    void clearItems();
    void loadGifsFromFolder(const QString &folder_path);
    bool isWidgetVisibleInViewport(QWidget *w);
    bool showDeleteConfirmationDialog(const QString &file_name);

    struct GifItem {
        ClickableLabel *label = nullptr;
        QMovie *movie = nullptr;
    };

    QVector<GifItem> items;
    QString current_folder;
    QStringList all_gif_files;
    QTimer *search_timer = nullptr;
    QTimer *visibility_timer = nullptr;
    QThread *loader_thread = nullptr;
    GifLoaderWorker *current_worker = nullptr;
    QMutex items_mutex;
    QMutex files_mutex;
    bool is_loading = false;
    quint64 current_load_id = 0;
    const QSize thumbnail_size = QSize(160, 120);
    const int columns = 3;
};

#endif // MAINWINDOW_H
