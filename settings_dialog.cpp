#include "settings_dialog.h"
#include "settings.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(SettingsManager &mgr, QWidget *parent)
    : QDialog(parent)
    , settings(mgr)
{
    setWindowTitle(tr("Settings"));
    setModal(true);

    QSize thumb_size = settings.getThumbnailSize();

    width_spin = new QSpinBox(this);
    width_spin->setRange(32, 512);
    width_spin->setValue(thumb_size.width());

    height_spin = new QSpinBox(this);
    height_spin->setRange(32, 512);
    height_spin->setValue(thumb_size.height());

    columns_spin = new QSpinBox(this);
    columns_spin->setRange(1, 8);
    columns_spin->setValue(settings.getColumns());

    auto *form = new QFormLayout;
    form->addRow(tr("Thumbnail width:"), width_spin);
    form->addRow(tr("Thumbnail height:"), height_spin);
    form->addRow(tr("Columns:"), columns_spin);

    default_folder_edit = new QLineEdit(this);
    default_folder_edit->setText(settings.getDefaultFolder());
    browse_folder_btn = new QPushButton(tr("Browse..."), this);
    connect(browse_folder_btn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Default Folder"),
                                                         default_folder_edit->text());
        if (!dir.isEmpty())
            default_folder_edit->setText(dir);
    });
    auto *folder_layout = new QHBoxLayout;
    folder_layout->addWidget(default_folder_edit);
    folder_layout->addWidget(browse_folder_btn);
    form->addRow(tr("Default folder:"), folder_layout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        settings.setThumbnailSize(QSize(width_spin->value(), height_spin->value()));
        settings.setColumns(columns_spin->value());
        settings.setDefaultFolder(default_folder_edit->text());
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *resetBtn = buttons->addButton(tr("Reset to Defaults"), QDialogButtonBox::ResetRole);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        settings.resetToDefaults();
        QSize def = settings.getThumbnailSize();
        width_spin->setValue(def.width());
        height_spin->setValue(def.height());
        columns_spin->setValue(settings.getColumns());
        default_folder_edit->setText(settings.getDefaultFolder());
    });

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}
