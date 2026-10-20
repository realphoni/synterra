#pragma once
#include "dockconfig.h"
#include <QWidget>

class QSlider;
class QSpinBox;
class QCheckBox;
class DockPreview;

class DockSettingsWidget final : public QWidget {
    Q_OBJECT
public:
    explicit DockSettingsWidget(QWidget *parent = nullptr);
    DockPreferences preferences() const;
    void setPreferences(const DockPreferences &preferences);
    void load();
    bool save();
signals:
    void edited();
private:
    QSpinBox *m_scale, *m_width, *m_spacing, *m_margin;
    QCheckBox *m_clock, *m_branding, *m_windows;
    DockPreview *m_preview;
    bool m_loading = false;
    void changed();
};

int showDockSettings();
