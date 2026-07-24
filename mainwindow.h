#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clickable_label.h"
#include "gif_loader_worker.h"
#include "settings.h"
#include "gif_viewer.h"
#include <QCloseEvent>
#include <QMainWindow>
#include <QPointer>
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
    void openFullSizeGif(const QString &file_path);
    void deleteGif(const QString &file_path);
    void renameGif(const QString &file_path);
    void onWorkerFinished();
    void goToDefaultFolder();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    void onGifLoaded(quint64 load_id, const LoadedGifData &data);
    void onLoadingFinished(quint64 load_id);
    void safeStopLoading();
    void scanToGif(const QString &path);
    void loadGifsFromFolder(const QString &folder_path);
    void copyGifUnderMouse();
    bool showDeleteConfirmationDialog(const QString &file_name);
    void openSettingsDialog();

    QString current_folder;
    QStringList all_gif_files;
    QPointer<GifViewer> active_viewer;
    QTimer *search_timer = nullptr;
    QTimer *visibility_timer = nullptr;
    QThread *loader_thread = nullptr;
    GifLoaderWorker *current_worker = nullptr;
    QMutex files_mutex;
    bool is_loading = false;
    quint64 current_load_id = 0;
};

#endif // MAINWINDOW_H
