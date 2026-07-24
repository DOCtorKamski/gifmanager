#include "gif_thumbnail_grid.h"
#include "settings.h"
#include <QFileInfo>
#include <QMovie>
#include <QScrollBar>

GifThumbnailGrid::GifThumbnailGrid(QWidget *parent)
    : QWidget(parent)
    , columns(SettingsManager::instance().getColumns())
    , thumbnail_size(SettingsManager::instance().getThumbnailSize())
    , h_spacing(SettingsManager::instance().getGridHorizontalSpacing())
    , v_spacing(SettingsManager::instance().getGridVerticalSpacing())
    , thumbnail_style(SettingsManager::instance().getThumbnailStyle())
{
    grid_layout = new QGridLayout(this);
    grid_layout->setContentsMargins(0, 0, 0, 0);
    grid_layout->setHorizontalSpacing(h_spacing);
    grid_layout->setVerticalSpacing(v_spacing);
    setLayout(grid_layout);
}

void GifThumbnailGrid::setScrollArea(QScrollArea *area)
{
    scroll_area = area;
}

void GifThumbnailGrid::applySettings()
{
    columns = SettingsManager::instance().getColumns();
    thumbnail_size = SettingsManager::instance().getThumbnailSize();
    thumbnail_style = SettingsManager::instance().getThumbnailStyle();
    h_spacing = SettingsManager::instance().getGridHorizontalSpacing();
    v_spacing = SettingsManager::instance().getGridVerticalSpacing();

    grid_layout->setHorizontalSpacing(h_spacing);
    grid_layout->setVerticalSpacing(v_spacing);
}

// --- GifItem --------------------

GifThumbnailGrid::GifItem::GifItem(ClickableLabel *lbl)
    : label(lbl), movie(nullptr)
{}

GifThumbnailGrid::GifItem::~GifItem()
{
    if (movie) {
        movie->stop();
        delete movie;
    }
    if (label)
        label->deleteLater();
}

GifThumbnailGrid::GifItem::GifItem(GifItem &&other) noexcept
    : label(std::exchange(other.label, nullptr))
    , movie(std::exchange(other.movie, nullptr))
{}

GifThumbnailGrid::GifItem& GifThumbnailGrid::GifItem::operator=(GifItem &&other) noexcept
{
    if (this != &other) {
        if (movie) {
            movie->stop();
            delete movie;
        }
        if (label)
            label->deleteLater();

        label = std::exchange(other.label, nullptr);
        movie = std::exchange(other.movie, nullptr);
    }
    return *this;
}

void GifThumbnailGrid::GifItem::createMovie(QWidget *parent, const QSize &scaledSize)
{
    if (movie)
        return;

    movie = new QMovie(label->filePath());
    movie->setParent(parent);
    movie->setCacheMode(QMovie::CacheNone);
    movie->setScaledSize(scaledSize);
    movie->jumpToFrame(0);
    movie->setPaused(true);

    connect(movie, &QMovie::frameChanged, label, [lbl = label, mov = movie](){
        QImage img = mov->currentImage();
        if (!img.isNull()) {
            QPixmap pm = QPixmap::fromImage(img.scaled(lbl->size(),
                                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
            lbl->setPixmap(pm);
        }
    });
}

void GifThumbnailGrid::GifItem::startMovie()
{
    if (!movie)
        return;
    if (movie->state() != QMovie::Running) {
        movie->setPaused(false);
        movie->start();
    }
}

void GifThumbnailGrid::GifItem::clearMovie()
{
    if (!movie)
        return;
    movie->stop();
    delete movie;
    movie = nullptr;
}

// --- Grid methods ------------------------

ClickableLabel *GifThumbnailGrid::createLabel(const LoadedGifData &data)
{
    ClickableLabel *lbl = new ClickableLabel(this);
    lbl->setFixedSize(thumbnail_size);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(thumbnail_style);
    lbl->setFilePath(data.file_path);
    lbl->setPixmap(data.thumbnail);

    connect(lbl, &ClickableLabel::copyRequested,
            this, &GifThumbnailGrid::copyRequested);
    connect(lbl, &ClickableLabel::openFullSizeRequested,
            this, &GifThumbnailGrid::openFullSizeRequested);
    connect(lbl, &ClickableLabel::deleteRequested,
            this, &GifThumbnailGrid::deleteRequested);
    connect(lbl, &ClickableLabel::renameRequested,
            this, &GifThumbnailGrid::renameRequested);

    return lbl;
}

void GifThumbnailGrid::addGif(const LoadedGifData &data)
{
    QMutexLocker locker(&items_mutex);

    ClickableLabel *lbl = createLabel(data);
    grid_layout->addWidget(lbl, items.size() / columns, items.size() % columns);
    items.emplace_back(lbl);
    adjustSize();
}

void GifThumbnailGrid::clearGrid()
{
    QMutexLocker locker(&items_mutex);
    items.clear();

    QLayoutItem *child;
    while ((child = grid_layout->takeAt(0)) != nullptr)
        delete child;
}

void GifThumbnailGrid::filterByQuery(const QString &query)
{
    QMutexLocker locker(&items_mutex);

    while (grid_layout->count() > 0)
        delete grid_layout->takeAt(0);

    int col = 0;
    int row = 0;
    for (auto &item : items) {
        if (!item.label)
            continue;

        bool matches = query.isEmpty()
            || QFileInfo(item.label->filePath()).fileName().toLower().contains(query);

        if (matches) {
            grid_layout->addWidget(item.label, row, col);
            item.label->setVisible(true);
            if (++col >= columns) {
                col = 0;
                ++row;
            }
        } else {
            item.label->setVisible(false);
        }
    }
    adjustSize();
}

void GifThumbnailGrid::releaseItem(const QString &file_path)
{
    QMutexLocker locker(&items_mutex);
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->label && it->label->filePath() == file_path) {
            grid_layout->removeWidget(it->label);
            items.erase(it);
            return;
        }
    }
}

void GifThumbnailGrid::playVisibleAnimations()
{
    QMutexLocker locker(&items_mutex);

    for (auto &item : items) {
        if (!item.label)
            continue;

        if (isWidgetVisibleInViewport(item.label)) {
            if (!item.hasMovie())
                item.createMovie(this, thumbnail_size);
            item.startMovie();
        } else {
            item.clearMovie();
        }
    }
}

bool GifThumbnailGrid::isWidgetVisibleInViewport(QWidget *w) const
{
    if (!w || !w->isVisible() || !scroll_area)
        return false;
    QRect viewport_rect = scroll_area->viewport()->rect();
    QPoint w_top_left = w->mapTo(scroll_area->viewport(), QPoint(0, 0));
    QRect widgetRect(w_top_left, w->size());
    return viewport_rect.intersects(widgetRect);
}

QString GifThumbnailGrid::filePathUnderMouse() const
{
    QMutexLocker locker(&items_mutex);
    for (const auto &item : items) {
        if (item.label && item.label->underMouse())
            return item.label->filePath();
    }
    return {};
}
