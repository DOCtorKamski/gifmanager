#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gif_viewer.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
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

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);

    search_timer = new QTimer(this);
    search_timer->setSingleShot(true);
    search_timer->setInterval(200);
    connect(search_timer, &QTimer::timeout, this, [this](){
        if (!is_loading && !current_folder.isEmpty()) {
            loadGifsFromFolder(current_folder);
        }
    });

    visibility_timer = new QTimer(this);
    visibility_timer->setSingleShot(true);
    visibility_timer->setInterval(80);
    connect(visibility_timer, &QTimer::timeout, this, &MainWindow::animateIfVisible);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString&){
        if (!is_loading) {
            search_timer->start();
        }
    });

    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            visibility_timer, qOverload<>(&QTimer::start));
    connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            visibility_timer, qOverload<>(&QTimer::start));

    auto *copy_shortcut = new QShortcut(QKeySequence(tr("Ctrl+C")), this);
    connect(copy_shortcut, &QShortcut::activated, this, &MainWindow::copyGifUnderMouse);

    auto *search_shortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
    connect(search_shortcut, &QShortcut::activated, this, [this]() {
        ui->searchEdit->setFocus();
        ui->searchEdit->selectAll();
    });
}

void MainWindow::chooseFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select GIF Folder"));
    if (!dir.isEmpty()) {
        current_folder = dir;
        QDir directory(dir);
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

void MainWindow::clearItems() {
    QMutexLocker locker(&items_mutex);

    for (auto &item : items) {
        if (item.movie) {
            item.movie->stop();
            item.movie->deleteLater();
        }
        if (item.label) {
            item.label->deleteLater();
        }
    }
    items.clear();

    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        delete child;
    }
}

void MainWindow::loadGifsFromFolder(const QString &path) {
    if (is_loading) {
        qDebug() << "Already loading, stopping current task...";
        safeStopLoading();
    }

    qDebug() << "Starting load from:" << path;

    clearItems();

    is_loading = true;
    current_load_id++;

    QString query = ui->searchEdit ? ui->searchEdit->text().trimmed() : "";

    QStringList files_to_load;
    {
        QMutexLocker locker(&files_mutex);
        files_to_load = all_gif_files;
    }

    loader_thread = new QThread(this);
    current_worker = new GifLoaderWorker(files_to_load, thumbnail_size, query, current_load_id);
    current_worker->moveToThread(loader_thread);

    connect(loader_thread, &QThread::started, current_worker, &GifLoaderWorker::process);

    connect(current_worker, QOverload<quint64, const LoadedGifData&>::of(&GifLoaderWorker::gifLoaded),
            this, &MainWindow::onGifLoaded, Qt::QueuedConnection);

    connect(current_worker, &GifLoaderWorker::progress, this, [this](int current, int total) {
        if (!is_loading) return;
        ui->statusbar->showMessage(QString("Loading: %1/%2").arg(current).arg(total));
    }, Qt::QueuedConnection);

    connect(current_worker, QOverload<quint64>::of(&GifLoaderWorker::finished),
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

    QMutexLocker locker(&items_mutex);

    ClickableLabel *label = new ClickableLabel(this);
    label->setFixedSize(thumbnail_size);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background: #999; border: 1px solid #444;");
    label->setFilePath(data.file_path);
    label->setPixmap(data.thumbnail);

    connect(label, &ClickableLabel::copyRequested, this, &MainWindow::copyGifToClipboard);
    connect(label, &ClickableLabel::openFullSizeRequested, this, &MainWindow::openFullSizeGif);
    connect(label, &ClickableLabel::deleteRequested, this, &MainWindow::deleteGif);
    connect(label, &ClickableLabel::renameRequested, this, &MainWindow::renameGif);

    int count = items.size();
    int row = count / columns;
    int col = count % columns;
    ui->gridLayout->addWidget(label, row, col);

    GifItem gi;
    gi.label = label;
    gi.movie = nullptr;
    items.append(gi);

    ui->containerWidget->adjustSize();
}

void MainWindow::onLoadingFinished(quint64 load_id) {
    if (load_id != current_load_id) {
        qDebug() << "Ignoring finished from outdated task:" << load_id << "current:" << current_load_id;
        return;
    }

    is_loading = false;
    ui->statusbar->showMessage("Ready");
    visibility_timer->start();
    qDebug() << "Loading finished";
}

bool MainWindow::isWidgetVisibleInViewport(QWidget *w) const {
    if (!w) return false;
    QRect viewport_rect = ui->scrollArea->viewport()->rect();
    QPoint w_top_left = w->mapTo(ui->scrollArea->viewport(), QPoint(0, 0));
    QRect widgetRect(w_top_left, w->size());
    return viewport_rect.intersects(widgetRect);
}

void MainWindow::animateIfVisible() {
    QMutexLocker locker(&items_mutex);

    for (auto &item : items) {
        if (!item.label) continue;

        bool is_visible = isWidgetVisibleInViewport(item.label);

        if (is_visible) {
            if (!item.movie) {
                lazyInitMovie(item);
            }
            if (item.movie) {
                bool is_running = (item.movie->state() == QMovie::Running);
                if (!is_running) {
                    item.movie->setPaused(false);
                    item.movie->start();
                }
            }
        } else {
            if (item.movie) {
                item.movie->stop();
                delete item.movie;
                item.movie = nullptr;
            }
        }
    }
}

void MainWindow::lazyInitMovie(GifItem &item) {
    QMovie *movie = new QMovie(item.label->filePath());
    movie->setParent(this);
    movie->setCacheMode(QMovie::CacheNone);
    movie->setScaledSize(thumbnail_size);
    movie->jumpToFrame(0);
    movie->setPaused(true);

    connect(movie, &QMovie::frameChanged, item.label, [label = item.label, movie](){
        QImage img = movie->currentImage();
        if (!img.isNull()) {
            QPixmap pm = QPixmap::fromImage(img.scaled(label->size(),
                                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
            label->setPixmap(pm);
        }
    });

    item.movie = movie;
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
    GifViewer *viewer = new GifViewer(file_path, this);
    connect(viewer, &GifViewer::copyRequested, this, &MainWindow::copyGifToClipboard);
    connect(viewer, &GifViewer::deleteRequested, this, &MainWindow::deleteGif);
    connect(viewer, &GifViewer::renameRequested, this, &MainWindow::renameGif);
    viewer->show();
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
    releaseGifItem(file_path);

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
    QString path;
    {
        QMutexLocker locker(&items_mutex);
        for (const auto &item : items) {
            if (item.label && item.label->underMouse()) {
                path = item.label->filePath();
                break;
            }
        }
    }
    if (!path.isEmpty())
        copyGifToClipboard(path);
}

void MainWindow::releaseGifItem(const QString &file_path) {
    QMutexLocker locker(&items_mutex);
    for (int i = items.size() - 1; i >= 0; --i) {
        if (items[i].label && items[i].label->filePath() == file_path) {
            if (items[i].movie) {
                items[i].movie->stop();
                delete items[i].movie;
                items[i].movie = nullptr;
            }
            ui->gridLayout->removeWidget(items[i].label);
            items[i].label->deleteLater();
            items.removeAt(i);
        }
    }
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
        releaseGifItem(file_path);

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

MainWindow::~MainWindow()
{
    safeStopLoading();
    delete ui;
}
