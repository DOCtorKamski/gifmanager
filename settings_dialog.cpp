#include "settings_dialog.h"
#include "settings.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setModal(true);

    QSize thumb_size = SettingsManager::instance().getThumbnailSize();

    width_spin = new QSpinBox;
    width_spin->setRange(32, 512);
    width_spin->setValue(thumb_size.width());

    height_spin = new QSpinBox;
    height_spin->setRange(32, 512);
    height_spin->setValue(thumb_size.height());

    columns_spin = new QSpinBox;
    columns_spin->setRange(1, 8);
    columns_spin->setValue(SettingsManager::instance().getColumns());

    default_folder_edit = new QLineEdit;
    default_folder_edit->setText(SettingsManager::instance().getDefaultFolder());
    browse_folder_btn = new QPushButton(tr("Browse..."));
    connect(browse_folder_btn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Default Folder"),
                                                         default_folder_edit->text());
        if (!dir.isEmpty())
            default_folder_edit->setText(dir);
    });

    auto *base_form = new QFormLayout;
    base_form->addRow(tr("Thumbnail width:"), width_spin);
    base_form->addRow(tr("Thumbnail height:"), height_spin);
    base_form->addRow(tr("Columns:"), columns_spin);
    auto *folder_layout = new QHBoxLayout;
    folder_layout->addWidget(default_folder_edit);
    folder_layout->addWidget(browse_folder_btn);
    base_form->addRow(tr("Default folder:"), folder_layout);

    auto *base_tab = new QWidget;
    base_tab->setLayout(base_form);

    search_debounce_spin = new QSpinBox;
    search_debounce_spin->setRange(50, 1000);
    search_debounce_spin->setValue(SettingsManager::instance().getSearchDebounceMs());
    search_debounce_spin->setSuffix(" ms");

    visibility_debounce_spin = new QSpinBox;
    visibility_debounce_spin->setRange(20, 500);
    visibility_debounce_spin->setValue(SettingsManager::instance().getVisibilityDebounceMs());
    visibility_debounce_spin->setSuffix(" ms");

    viewer_width_spin = new QSpinBox;
    viewer_width_spin->setRange(400, 1920);
    viewer_width_spin->setValue(SettingsManager::instance().getViewerSize().width());

    viewer_height_spin = new QSpinBox;
    viewer_height_spin->setRange(400, 1920);
    viewer_height_spin->setValue(SettingsManager::instance().getViewerSize().height());

    grid_hspacing_spin = new QSpinBox;
    grid_hspacing_spin->setRange(0, 20);
    grid_hspacing_spin->setValue(SettingsManager::instance().getGridHorizontalSpacing());

    grid_vspacing_spin = new QSpinBox;
    grid_vspacing_spin->setRange(0, 20);
    grid_vspacing_spin->setValue(SettingsManager::instance().getGridVerticalSpacing());

    auto *ext_form = new QFormLayout;
    ext_form->addRow(tr("Search debounce:"), search_debounce_spin);
    ext_form->addRow(tr("Visibility debounce:"), visibility_debounce_spin);
    ext_form->addRow(tr("Viewer width:"), viewer_width_spin);
    ext_form->addRow(tr("Viewer height:"), viewer_height_spin);
    ext_form->addRow(tr("Grid horizontal spacing:"), grid_hspacing_spin);
    ext_form->addRow(tr("Grid vertical spacing:"), grid_vspacing_spin);

    auto *ext_tab = new QWidget;
    ext_tab->setLayout(ext_form);

    auto *tabs = new QTabWidget;
    tabs->addTab(base_tab, tr("Base"));
    tabs->addTab(ext_tab, tr("Extended"));

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        writeSettings();
        emit applied();
        accept();
    });
    auto *applyBtn = buttons->button(QDialogButtonBox::Apply);
    connect(applyBtn, &QPushButton::clicked, this, [this]() {
        writeSettings();
        emit applied();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *resetBtn = buttons->addButton(tr("Reset to Defaults"), QDialogButtonBox::ResetRole);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::refreshUI);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
}

void SettingsDialog::writeSettings()
{
    SettingsManager::instance().setThumbnailSize(
        QSize(width_spin->value(), height_spin->value()));
    SettingsManager::instance().setColumns(columns_spin->value());
    SettingsManager::instance().setSearchDebounceMs(search_debounce_spin->value());
    SettingsManager::instance().setVisibilityDebounceMs(visibility_debounce_spin->value());
    SettingsManager::instance().setViewerSize(
        QSize(viewer_width_spin->value(), viewer_height_spin->value()));
    SettingsManager::instance().setGridHorizontalSpacing(grid_hspacing_spin->value());
    SettingsManager::instance().setGridVerticalSpacing(grid_vspacing_spin->value());
    SettingsManager::instance().setDefaultFolder(default_folder_edit->text());
}

void SettingsDialog::refreshUI()
{
    SettingsManager::instance().resetToDefaults();
    QSize def = SettingsManager::instance().getThumbnailSize();
    width_spin->setValue(def.width());
    height_spin->setValue(def.height());
    columns_spin->setValue(SettingsManager::instance().getColumns());
    search_debounce_spin->setValue(SettingsManager::instance().getSearchDebounceMs());
    visibility_debounce_spin->setValue(SettingsManager::instance().getVisibilityDebounceMs());
    viewer_width_spin->setValue(SettingsManager::instance().getViewerSize().width());
    viewer_height_spin->setValue(SettingsManager::instance().getViewerSize().height());
    grid_hspacing_spin->setValue(SettingsManager::instance().getGridHorizontalSpacing());
    grid_vspacing_spin->setValue(SettingsManager::instance().getGridVerticalSpacing());
    default_folder_edit->setText(SettingsManager::instance().getDefaultFolder());
}
