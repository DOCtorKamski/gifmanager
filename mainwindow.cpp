#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gif_thumbnail_grid.h"
#include "gif_viewer.h"
#include "settings_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>
#include <QScrollBar>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , current_folder("")
    , loader_thread(nullptr)
    , current_worker(nullptr)
    , is_loading(false)
    , current_load_id(0)
{
    ui->setupUi(this);

    ui->containerWidget->setScrollArea(ui->scrollArea);

    QByteArray geom = SettingsManager::instance().getWindowGeometry();
    if (!geom.isEmpty())
        restoreGeometry(geom);

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
    connect(ui->settingButton, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);
    connect(ui->defaultButton, &QPushButton::clicked, this, &MainWindow::goToDefaultFolder);

    search_timer = new QTimer(this);
    search_timer->setSingleShot(true);
    search_timer->setInterval(SettingsManager::instance().getSearchDebounceMs());
    connect(search_timer, &QTimer::timeout, this, [this](){
        if (!is_loading && !current_folder.isEmpty()) {
            ui->containerWidget->filterByQuery(ui->searchEdit->text().trimmed());
            ui->scrollArea->verticalScrollBar()->setValue(0);
            visibility_timer->start();
        }
    });

    visibility_timer = new QTimer(this);
    visibility_timer->setSingleShot(true);
    visibility_timer->setInterval(SettingsManager::instance().getVisibilityDebounceMs());
    connect(visibility_timer, &QTimer::timeout, this, [this]() {
        ui->containerWidget->playVisibleAnimations();
    });

    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString&){
        if (!is_loading) {
            search_timer->start();
        }
    });

    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            visibility_timer, qOverload<>(&QTimer::start));
    connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            visibility_timer, qOverload<>(&QTimer::start));

    connect(ui->containerWidget, &GifThumbnailGrid::copyRequested,
            this, &MainWindow::copyGifToClipboard);
    connect(ui->containerWidget, &GifThumbnailGrid::openFullSizeRequested,
            this, &MainWindow::openFullSizeGif);
    connect(ui->containerWidget, &GifThumbnailGrid::deleteRequested,
            this, &MainWindow::deleteGif);
    connect(ui->containerWidget, &GifThumbnailGrid::renameRequested,
            this, &MainWindow::renameGif);

    auto *copy_shortcut = new QShortcut(QKeySequence(tr("Ctrl+C")), this);
    connect(copy_shortcut, &QShortcut::activated, this, &MainWindow::copyGifUnderMouse);

    auto *search_shortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
    connect(search_shortcut, &QShortcut::activated, this, [this]() {
        ui->searchEdit->setFocus();
        ui->searchEdit->selectAll();
    });

}

void MainWindow::chooseFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select GIF Folder"), current_folder);
    if (!dir.isEmpty()) {
        scanToGif(dir);
    }
}

void MainWindow::scanToGif(const QString &path) {
    current_folder = path;
    QDir directory(path);
    directory.setNameFilters({"*.gif", "*.GIF"});
    QFileInfoList files = directory.entryInfoList(QDir::Files, QDir::Name);
    {
        QMutexLocker locker(&files_mutex);
        all_gif_files.clear();
        for (const QFileInfo &fi : std::as_const(files)) {
            all_gif_files.append(fi.absoluteFilePath());
        }
    }
    ui->searchEdit->clear();
    loadGifsFromFolder(current_folder);
}

void MainWindow::safeStopLoading() {
    search_timer->stop();
    visibility_timer->stop();

    if (current_worker) {
        current_worker->requestStop();
    }

    if (loader_thread) {
        if (loader_thread->isRunning()) {
            if (!loader_thread->wait(5000)) {
                qWarning() << "Loader thread did not stop within timeout";
            }
        }
        delete loader_thread;
        loader_thread = nullptr;
    }

    current_worker = nullptr;
    is_loading = false;
}

void MainWindow::loadGifsFromFolder(const QString &path) {
    if (is_loading) {
        qDebug() << "Already loading, stopping current task...";
        safeStopLoading();
    }

    qDebug() << "Starting load from:" << path;

    ui->containerWidget->clearGrid();

    is_loading = true;
    current_load_id++;

    QStringList files_to_load;
    {
        QMutexLocker locker(&files_mutex);
        files_to_load = all_gif_files;
    }

    loader_thread = new QThread(this);
    current_worker = new GifLoaderWorker(files_to_load, SettingsManager::instance().getThumbnailSize(), current_load_id);
    current_worker->moveToThread(loader_thread);

    connect(loader_thread, &QThread::started, current_worker, &GifLoaderWorker::process);

    connect(current_worker, &GifLoaderWorker::gifLoaded,
            this, &MainWindow::onGifLoaded, Qt::QueuedConnection);

    connect(current_worker, &GifLoaderWorker::progress, this, [this](int current, int total) {
        if (!is_loading) return;
        ui->statusbar->showMessage(QString("Loading: %1/%2").arg(current).arg(total));
    }, Qt::QueuedConnection);

    connect(current_worker, &GifLoaderWorker::finished,
            this, &MainWindow::onLoadingFinished, Qt::QueuedConnection);

    connect(current_worker, &GifLoaderWorker::finished, this, [this, load_id = current_load_id]() {
        if (load_id != current_load_id) return;
        onWorkerFinished();
    });

    loader_thread->start();
}

void MainWindow::onWorkerFinished() {
    qDebug() << "Worker finished signal received";
    if (current_worker) {
        current_worker->deleteLater();
        current_worker = nullptr;
    }
    if (loader_thread) {
        loader_thread->quit();
        loader_thread->wait();
        loader_thread->deleteLater();
        loader_thread = nullptr;
    }
}

void MainWindow::onGifLoaded(quint64 load_id, const LoadedGifData &data) {
    if (load_id != current_load_id || !is_loading) {
        return;
    }

    ui->containerWidget->addGif(data);
}

void MainWindow::onLoadingFinished(quint64 load_id) {
    if (load_id != current_load_id) {
        qDebug() << "Ignoring finished from outdated task:" << load_id << "current:" << current_load_id;
        return;
    }

    is_loading = false;
    ui->statusbar->showMessage("Ready");

    QString query = ui->searchEdit ? ui->searchEdit->text().trimmed() : QString();
    if (!query.isEmpty()) {
        ui->containerWidget->filterByQuery(query);
    }

    visibility_timer->start();
    qDebug() << "Loading finished";
}

void MainWindow::copyGifToClipboard(const QString &file_path) {
    QMimeData *mime = new QMimeData;

    mime->setData("application/x-qt-windows-mime;value=\"FileNameW\"",
                  QFileInfo(file_path).fileName().toUtf8());

    QImageReader reader(file_path);
    QImage img = reader.read();
    if (!img.isNull()) {
        mime->setImageData(QVariant::fromValue(QPixmap::fromImage(img)));
    }

    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(file_path));
    mime->setUrls(urls);

    QClipboard *cb = QApplication::clipboard();
    cb->setMimeData(mime);
}

void MainWindow::openFullSizeGif(const QString &file_path) {
    if (active_viewer) {
        if (active_viewer->filePath() == file_path) {
            active_viewer->close();
            return;
        }
        active_viewer->close();
    }

    GifViewer *viewer = new GifViewer(file_path, this);
    connect(viewer, &GifViewer::copyRequested, this, &MainWindow::copyGifToClipboard);
    connect(viewer, &GifViewer::deleteRequested, this, &MainWindow::deleteGif);
    connect(viewer, &GifViewer::renameRequested, this, &MainWindow::renameGif);
    viewer->show();
    active_viewer = viewer;
}

void MainWindow::deleteGif(const QString &file_path) {
    QFileInfo file_info(file_path);

    if (!file_info.exists()) {
        qWarning() << "File does not exist:" << file_path;
        QMessageBox::critical(this, tr("Error"), tr("File does not exist."));
        return;
    }

    if (!showDeleteConfirmationDialog(file_info.fileName())) {
        return;
    }

    //this need for Windows
    ui->containerWidget->releaseItem(file_path);

    QFile file(file_path);
    if (!file.remove()) {
        qWarning() << "Failed to delete file:" << file_path;
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to delete file: %1").arg(file_info.fileName()));
        return;
    }

    qInfo() << "File deleted:" << file_path;

    {
        QMutexLocker locker(&files_mutex);
        all_gif_files.removeAll(file_path);
    }

    loadGifsFromFolder(current_folder);
}

void MainWindow::copyGifUnderMouse() {
    QString path = ui->containerWidget->filePathUnderMouse();
    if (!path.isEmpty())
        copyGifToClipboard(path);
}

bool MainWindow::showDeleteConfirmationDialog(const QString &file_name) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Deletion"),
        tr("Are you sure you want to delete '%1'?").arg(file_name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );
    return reply == QMessageBox::Yes;
}

void MainWindow::renameGif(const QString &file_path) {
    QFileInfo fi(file_path);
    QString old_name = fi.baseName();
    QString extension = fi.suffix();

    bool ok{};
    QString new_name = QInputDialog::getText(
        this,
        tr("Rename GIF"),
        tr("Enter new name (without extension):"),
        QLineEdit::Normal,
        old_name,
        &ok
    );

    if (ok && !new_name.isEmpty() && new_name != old_name) {
        if (new_name.contains(QRegularExpression("[/\\\\:*?\"<>|]"))) {
            QMessageBox::warning(this, tr("Invalid Name"),
                                 tr("Name contains invalid characters"));
            return;
        }

        QString new_file_path = QDir(fi.absolutePath()).filePath(new_name + "." + extension);

        if (QFile::exists(new_file_path)) {
            QMessageBox::warning(this, tr("File Exists"),
                                 tr("A file with this name already exists"));
            return;
        }

        //this need for Windows
        ui->containerWidget->releaseItem(file_path);

        if (QFile::rename(file_path, new_file_path)) {
            {
                QMutexLocker locker(&files_mutex);
                all_gif_files.removeAll(file_path);
                all_gif_files.append(new_file_path);
            }

            loadGifsFromFolder(current_folder);

            QMessageBox::information(this, tr("Success"),
                                     tr("File renamed successfully"));
        }
        else {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to rename file"));
        }
    }
}

void MainWindow::goToDefaultFolder() {
    QString dir = SettingsManager::instance().getDefaultFolder();
    if (dir.isEmpty()) {
        QMessageBox::information(this, tr("No Default Folder"),
                                 tr("No default folder is set. Please configure it in Settings."));
        return;
    }
    if (!QDir(dir).exists()) {
        QMessageBox::warning(this, tr("Folder Not Found"),
                             tr("The default folder '%1' does not exist.").arg(dir));
        return;
    }
    scanToGif(dir);
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dlg(this);
    connect(&dlg, &SettingsDialog::applied, this, [this]() {
        search_timer->setInterval(SettingsManager::instance().getSearchDebounceMs());
        visibility_timer->setInterval(SettingsManager::instance().getVisibilityDebounceMs());
        ui->containerWidget->applySettings();
        if (!current_folder.isEmpty())
            loadGifsFromFolder(current_folder);
    });
    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    SettingsManager::instance().setWindowGeometry(saveGeometry());
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    safeStopLoading();
    delete ui;
}
