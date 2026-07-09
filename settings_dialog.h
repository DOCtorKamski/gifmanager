#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void applied();

private:
    void writeSettings();
    void refreshUI();

    QSpinBox *width_spin;
    QSpinBox *height_spin;
    QSpinBox *columns_spin;
    QSpinBox *search_debounce_spin;
    QSpinBox *visibility_debounce_spin;
    QSpinBox *viewer_width_spin;
    QSpinBox *viewer_height_spin;
    QSpinBox *grid_hspacing_spin;
    QSpinBox *grid_vspacing_spin;
    QLineEdit *default_folder_edit;
    QPushButton *browse_folder_btn;
};

#endif
