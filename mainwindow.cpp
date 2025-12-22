#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMimeData> //for copyClipboard
#include <QClipboard> //for copyClipboard
#include "clickable_label.h"
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);

    visibility_timer = new QTimer(this);
    visibility_timer->setSingleShot(true);
    visibility_timer->setInterval(80); // fire signal every 80 milsec
    connect(visibility_timer, &QTimer::timeout, this, &MainWindow::animateIfVisible);

    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, visibility_timer, qOverload<>(&QTimer::start));
    connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, visibility_timer, qOverload<>(&QTimer::start));
}

void MainWindow::chooseFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select GIF Folder"));
    if (!dir.isEmpty()) {
        loadGifsFromFolder(dir);
    }
}

void MainWindow::loadGifsFromFolder(const QString &path) {
   // clearItems(); //TODO did this func, mby dont need. Clear before new folder choose

    QDir directory(path);
    QStringList filters;
    filters << "*.gif" << "*.GIF";
    QFileInfoList files = directory.entryInfoList(filters, QDir::Files, QDir::Name);

    int row = 0, col = 0;
    for (const QFileInfo &fi : files) {
        ClickableLabel *label = new ClickableLabel;
        label->setFixedSize(thumbnailSize);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("background: #222; border: 1px solid #444;"); //TODO change style
        label->setFilePath(fi.absoluteFilePath());
        connect(label, &ClickableLabel::copyRequested, this, &MainWindow::copyGifToClipboard);

        QMovie *movie = new QMovie(fi.absoluteFilePath());
        movie->setCacheMode(QMovie::CacheAll);
        movie->setScaledSize(thumbnailSize);

        // resize GIFs to default size
        connect(movie, &QMovie::frameChanged, label, [label, movie, this](int){
            QImage img = movie->currentImage();
            if (!img.isNull()) {
                QPixmap pm = QPixmap::fromImage(img.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                label->setPixmap(pm);
            }
        });

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
void MainWindow::copyGifToClipboard(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    QMimeData *mime = new QMimeData;
    // Raw GIF bytes
    mime->setData("image/gif", data);

    // Also provide generic bytes and a filename hint
    mime->setData("application/octet-stream", data);
    mime->setData("application/x-qt-windows-mime;value=\"FileNameW\"",
                  QFileInfo(filePath).fileName().toUtf8());

    // Provide a pixmap fallback (first frame) for apps that only accept images
    QMovie tmpMovie(filePath);
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
    urls.append(QUrl::fromLocalFile(filePath));
    mime->setUrls(urls);

    QClipboard *cb = QApplication::clipboard();
    cb->setMimeData(mime);
}

MainWindow::~MainWindow()
{
    delete ui;
}

