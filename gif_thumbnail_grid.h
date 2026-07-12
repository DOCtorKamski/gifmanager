#ifndef GIFTHUMBNAILGRID_H
#define GIFTHUMBNAILGRID_H

#include "clickable_label.h"
#include "gif_loader_worker.h"
#include <QGridLayout>
#include <QMutex>
#include <QScrollArea>
#include <QWidget>
#include <vector>

class QMovie;

class GifThumbnailGrid : public QWidget
{
    Q_OBJECT
public:
    explicit GifThumbnailGrid(QWidget *parent = nullptr);

    void addGif(const LoadedGifData &data);
    void clearGrid();
    void releaseItem(const QString &file_path);
    void playVisibleAnimations();
    void setScrollArea(QScrollArea *area);
    void applySettings();
    QString filePathUnderMouse() const;

signals:
    void copyRequested(const QString &file_path);
    void deleteRequested(const QString &file_path);
    void openFullSizeRequested(const QString &file_path);
    void renameRequested(const QString &file_path);

private:
    class GifItem {
    public:
        explicit GifItem(ClickableLabel *lbl);
        ~GifItem();

        GifItem(const GifItem&) = delete;
        GifItem& operator=(const GifItem&) = delete;
        GifItem(GifItem &&other) noexcept;
        GifItem& operator=(GifItem &&other) noexcept;

        void createMovie(QWidget *parent, const QSize &scaledSize);
        void startMovie();
        void clearMovie();
        bool hasMovie() const { return movie != nullptr; }

        ClickableLabel *label = nullptr;
        QMovie *movie = nullptr;
    };

    bool isWidgetVisibleInViewport(QWidget *w) const;

    std::vector<GifItem> items;
    QGridLayout *grid_layout;
    QScrollArea *scroll_area = nullptr;
    mutable QMutex items_mutex;
};

#endif // GIFTHUMBNAILGRID_H
