#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clickable_label.h"
#include "gif_viewer.h"
#include <QLabel>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMimeData> //for copyClipboard
#include <QClipboard> //for copyClipboard
#include <QScrollBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , current_folder("")
{
    ui->setupUi(this);

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString&){ //use labda instead slots
        search_timer->start();
    });

    visibility_timer = new QTimer(this);
    visibility_timer->setSingleShot(true);
    visibility_timer->setInterval(80); // fire signal every 80 milsec
    connect(visibility_timer, &QTimer::timeout, this, &MainWindow::animateIfVisible);

    search_timer = new QTimer(this);
    search_timer->setSingleShot(true);
    search_timer->setInterval(200);
    connect(search_timer, &QTimer::timeout, this, [this](){ //use labda instead slots
        loadGifsFromFolder(current_folder);
    });

    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, visibility_timer, qOverload<>(&QTimer::start));
    connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, visibility_timer, qOverload<>(&QTimer::start));
}

void MainWindow::chooseFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select GIF Folder"));
    if (!dir.isEmpty()) {
        current_folder = dir;
        QDir directory(dir);
        QStringList filters;
        directory.setNameFilters(QStringList() << "*.gif" << "*.GIF");
        QFileInfoList files = directory.entryInfoList(QDir::Files, QDir::Name);
        all_gif_files.clear();
        for (const QFileInfo &fi : files) all_gif_files.append(fi.absoluteFilePath());
        ui->searchEdit->clear();
        loadGifsFromFolder(current_folder);
    }
}

//Googled this func
void MainWindow::clearItems() {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->movie) {
            it->movie->stop();
            it->movie->deleteLater();
        }
        if (it->label) {
            it->label->deleteLater();
        }
    }
    items.clear();

    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        delete child;
    }
}

void MainWindow::loadGifsFromFolder(const QString &path) {
    clearItems();

    QString query;
    if (ui->searchEdit) query = ui->searchEdit->text().trimmed().toLower();
    const bool has_query = !query.isEmpty();

    int row = 0, col = 0;
    for (const QString &file_path : all_gif_files) {
        QFileInfo fi(file_path);
        if (!fi.exists() || !fi.isFile()) continue;

        if (has_query) {
            if (!fi.fileName().toLower().contains(query)) continue;
        }

        //TODO create new func for this
        ClickableLabel *label = new ClickableLabel(this);
        label->setFixedSize(thumbnailSize);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("background: #222; border: 1px solid #444;"); //TODO change style
        label->setFilePath(file_path);
        connect(label, &ClickableLabel::copyRequested, this, &MainWindow::copyGifToClipboard);
        connect(label, &ClickableLabel::openFullSizeRequested, this, &MainWindow::openFullSizeGif);
        connect(label, &ClickableLabel::renameRequested, this, &MainWindow::renameGif);

        QMovie *movie = new QMovie(file_path);
        movie->setCacheMode(QMovie::CacheAll);
        movie->setScaledSize(thumbnailSize);
        movie->jumpToFrame(0);
        movie->setPaused(true);

        // resize GIFs to default size
        connect(movie, &QMovie::frameChanged, label, [label, movie, this](int){
            QImage img = movie->currentImage();
            if (!img.isNull()) {
                QPixmap pm = QPixmap::fromImage(img.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                label->setPixmap(pm);
            }
        });
        //TODO create new func for this

        ui->gridLayout->addWidget(label, row, col);
        GifItem gi;
        gi.label = label;
        gi.movie = movie;
        items.insert(label, gi);

        col++;
        if (col >= columns) { col = 0; row++; }
    }

    ui->containerWidget->adjustSize();
    visibility_timer->start();
}

bool MainWindow::isWidgetVisibleInViewport(QWidget *w) {
    if (!w) return false;
    QRect viewport_rect = ui->scrollArea->viewport()->rect();
    QPoint w_top_left = w->mapTo(ui->scrollArea->viewport(), QPoint(0,0));
    QRect widgetRect(w_top_left, w->size());
    return viewport_rect.intersects(widgetRect);
}

void MainWindow::animateIfVisible() {
    for (auto &item : items) {
        if (!item.label || !item.movie) continue;

        bool is_visible = isWidgetVisibleInViewport(item.label);
        bool is_running = (item.movie->state() == QMovie::Running);

        if (is_visible && !is_running) {
            item.movie->setPaused(false);
            item.movie->start();
        } else if (!is_visible && is_running) {
            item.movie->setPaused(true);
        }
    }
}

// I Googled this func
void MainWindow::copyGifToClipboard(const QString &file_path) {
    QFile f(file_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    QMimeData *mime = new QMimeData;
    // Raw GIF bytes
    mime->setData("image/gif", data);

    // Also provide generic bytes and a filename hint
    mime->setData("application/octet-stream", data);
    mime->setData("application/x-qt-windows-mime;value=\"FileNameW\"",
                  QFileInfo(file_path).fileName().toUtf8());

    // Provide a pixmap fallback (first frame) for apps that only accept images
    QMovie tmpMovie(file_path);
    if (tmpMovie.isValid()) {
        tmpMovie.start();
        tmpMovie.jumpToFrame(0);
        QImage img = tmpMovie.currentImage();
        if (!img.isNull()) {
            mime->setImageData(QVariant::fromValue(QPixmap::fromImage(img)));
        }
        tmpMovie.stop();
    }

    // Also set a file URL so pasting into a file-aware target uses the original file
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(file_path));
    mime->setUrls(urls);

    QClipboard *cb = QApplication::clipboard();
    cb->setMimeData(mime);
}

void MainWindow::openFullSizeGif(const QString &file_path) {
    GifViewer *viewer = new GifViewer(file_path);
    connect(viewer, &GifViewer::copyRequested, this, &MainWindow::copyGifToClipboard);
    connect(viewer, &GifViewer::renameRequested, this, &MainWindow::renameGif);
    viewer->show();
}

void MainWindow::renameGif(const QString &file_path) {
    QFileInfo fi(file_path);
    QString old_name = fi.baseName();
    QString extension = fi.suffix();

    bool ok;
    QString new_name = QInputDialog::getText(this,
                                            tr("Rename GIF"),
                                            tr("Enter new name (without extension):"),
                                            QLineEdit::Normal,
                                            old_name,
                                            &ok);

    if (ok && !new_name.isEmpty() && new_name != old_name) {
        if (new_name.contains(QRegularExpression("[/\\\\:*?\"<>|]"))) {
            QMessageBox::warning(this, tr("Invalid Name"),
                                 tr("Name contains invalid characters"));
            return;
        }

        QString new_file_path = fi.absolutePath() + "/" + new_name + "." + extension;

        if (QFile::exists(new_file_path)) {
            QMessageBox::warning(this, tr("File Exists"),
                                 tr("A file with this name already exists"));
            return;
        }

        if (QFile::rename(file_path, new_file_path)) {
            all_gif_files.removeAll(file_path);
            all_gif_files.append(new_file_path);
            loadGifsFromFolder(current_folder);

            QMessageBox::information(this, tr("Success"),
                                     tr("File renamed successfully"));
        } else {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to rename file"));
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

