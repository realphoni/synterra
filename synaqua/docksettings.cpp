#include "docksettings.h"
#include <QApplication>
#include <QCheckBox>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QSettings settings() { return QSettings("Synterra", qApp->applicationName() == "SynaquaTest" ? "SynaquaTest" : "Synaqua"); }
}

class DockPreview final : public QWidget {
public:
    DockPreferences preferences;
    explicit DockPreview(QWidget *parent) : QWidget(parent) { setMinimumHeight(112); setAccessibleName("Dock layout preview"); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        const QRectF backdrop(0, 0, width(), height());
        QLinearGradient sky(backdrop.topLeft(), backdrop.bottomRight()); sky.setColorAt(0, QColor("#273857")); sky.setColorAt(1, QColor("#9381b2"));
        painter.setPen(Qt::NoPen); painter.setBrush(sky); painter.drawRoundedRect(backdrop, 12, 12);
        const qreal scale = preferences.scale / 100.;
        const qreal dockWidth = qMax(210., (width() - 24) * preferences.width / 100.);
        const qreal dockHeight = 44 * scale;
        const QRectF dock((width()-dockWidth)/2, height()-dockHeight-8-preferences.margin/3., dockWidth, dockHeight);
        QLinearGradient glass(dock.topLeft(), dock.bottomLeft()); glass.setColorAt(0, QColor("#deefff")); glass.setColorAt(.5, QColor("#94b2d0")); glass.setColorAt(1, QColor("#c7deef"));
        painter.setBrush(glass); painter.setPen(QColor("#e7f9ff")); painter.drawRoundedRect(dock, 12, 12);
        const qreal orb = 30*scale; const QPointF center = dock.center();
        const QRectF start(center.x()-orb/2,center.y()-orb/2,orb,orb);
        QLinearGradient blue(start.topLeft(),start.bottomLeft()); blue.setColorAt(0,QColor("#95d8fc")); blue.setColorAt(1,QColor("#1760a2"));
        painter.setBrush(blue); painter.setPen(QColor("#e5f7ff")); painter.drawEllipse(start);
        auto font=painter.font(); font.setPixelSize(qRound(orb*.7)); painter.setFont(font); painter.setPen(Qt::white); painter.drawText(start,Qt::AlignCenter,"S");
        const qreal step = (19 + preferences.spacing/2.)*scale;
        for (int side : {-1, 1}) for (int index=1; index<=3; ++index) {
            const qreal x=center.x()+side*(orb/2+index*step)-6*scale;
            painter.setBrush(index%2 ? QColor("#437cab") : QColor("#7198c6")); painter.setPen(QColor("#d9efff"));
            painter.drawRoundedRect(QRectF(x,center.y()-7*scale,12*scale,14*scale),3,3);
        }
    }
};

DockSettingsWidget::DockSettingsWidget(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this); layout->setContentsMargins(18,18,18,18); layout->setSpacing(14);
    auto title = new QLabel("<h2>Make the dock yours</h2>", this); layout->addWidget(title);
    auto intro = new QLabel("Choose a compact dock or stretch it across your desktop. Changes take effect when you Apply.", this);
    intro->setWordWrap(true); layout->addWidget(intro);
    m_preview = new DockPreview(this); layout->addWidget(m_preview);
    auto form = new QFormLayout; form->setSpacing(12); layout->addLayout(form);
    auto control = [&](const QString &label, const char *name, int low, int high, int step, const QString &suffix) {
        auto row = new QWidget(this); auto horizontal = new QHBoxLayout(row); horizontal->setContentsMargins(0,0,0,0);
        auto slider = new QSlider(Qt::Horizontal, row); slider->setRange(low,high); slider->setSingleStep(step); slider->setPageStep(step*2); slider->setAccessibleName(label);
        auto number = new QSpinBox(row); number->setRange(low,high); number->setSingleStep(step); number->setSuffix(suffix); number->setObjectName(name); number->setAccessibleName(label);
        horizontal->addWidget(slider,1); horizontal->addWidget(number);
        connect(slider,&QSlider::valueChanged,number,&QSpinBox::setValue);
        connect(number,qOverload<int>(&QSpinBox::valueChanged),slider,&QSlider::setValue);
        connect(number,qOverload<int>(&QSpinBox::valueChanged),this,[this] { changed(); });
        form->addRow(label,row); return number;
    };
    m_scale=control("Dock size", "dockScale",60,125,5," %");
    m_width=control("Dock width", "dockWidth",45,100,1," %");
    m_spacing=control("Icon spacing", "dockSpacing",0,16,1," px");
    m_margin=control("Distance from screen edge", "dockMargin",0,48,2," px");
    auto presets = new QHBoxLayout; presets->addWidget(new QLabel("Presets:",this));
    auto preset = [&](const QString &name, DockPreferences preferences) {
        auto button=new QPushButton(name,this); presets->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this,preferences] { setPreferences(preferences); emit edited(); });
    };
    DockPreferences compact; compact.scale=75; compact.width=70; compact.margin=12;
    DockPreferences wide; wide.width=100; wide.spacing=6; wide.margin=8;
    preset("Compact",compact); preset("Balanced",DockPreferences{}); preset("Wide",wide); presets->addStretch(); layout->addLayout(presets);
    m_clock=new QCheckBox("Show clock and date",this); m_clock->setObjectName("showClock");
    m_branding=new QCheckBox("Show Synaqua name when there is room",this);
    m_windows=new QCheckBox("Show running windows",this);
    for (auto box : {m_clock,m_branding,m_windows}) { layout->addWidget(box); connect(box,&QCheckBox::toggled,this,[this] { changed(); }); }
    auto note=new QLabel("The dock stays centered. It automatically leaves enough room for your pinned apps and keeps windows above it. Pins and wallpaper are preserved.",this);
    note->setWordWrap(true); layout->addWidget(note); layout->addStretch(); load();
}
DockPreferences DockSettingsWidget::preferences() const {
    return {m_scale->value(),m_width->value(),m_spacing->value(),m_margin->value(),m_clock->isChecked(),m_branding->isChecked(),m_windows->isChecked()};
}
void DockSettingsWidget::setPreferences(const DockPreferences &preferences) {
    m_loading=true;
    m_scale->setValue(preferences.scale); m_width->setValue(preferences.width); m_spacing->setValue(preferences.spacing); m_margin->setValue(preferences.margin);
    m_clock->setChecked(preferences.clock); m_branding->setChecked(preferences.branding); m_windows->setChecked(preferences.windows);
    m_loading=false; m_preview->preferences=this->preferences(); m_preview->update();
}
void DockSettingsWidget::changed() { if (!m_loading) { m_preview->preferences=preferences(); m_preview->update(); emit edited(); } }
void DockSettingsWidget::load() { auto config=settings(); setPreferences(DockPreferences::read(config)); }
bool DockSettingsWidget::save() {
    auto config=settings(); preferences().write(config);
    if(config.status()!=QSettings::NoError) return false;
    auto message=QDBusMessage::createMethodCall("org.synterra.Synaqua","/Synaqua","org.synterra.Synaqua","reloadDockSettings");
    QDBusConnection::sessionBus().asyncCall(message); return true;
}
int showDockSettings() {
    QDialog dialog; dialog.setWindowTitle("Synaqua Dock"); dialog.setWindowIcon(QIcon::fromTheme("synterra")); dialog.resize(650,650);
    auto layout=new QVBoxLayout(&dialog); auto editor=new DockSettingsWidget(&dialog); layout->addWidget(editor);
    auto buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::RestoreDefaults|QDialogButtonBox::Close,&dialog); layout->addWidget(buttons);
    auto apply=buttons->button(QDialogButtonBox::Apply); apply->setEnabled(false);
    QObject::connect(editor,&DockSettingsWidget::edited,&dialog,[apply] { apply->setEnabled(true); });
    QObject::connect(apply,&QPushButton::clicked,&dialog,[&dialog,editor,apply] {
        if(editor->save()) apply->setEnabled(false);
        else QMessageBox::warning(&dialog,"Synaqua Dock","The dock settings could not be saved. Check that your configuration folder is writable.");
    });
    QObject::connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,&dialog,[editor,apply] { editor->setPreferences(DockPreferences{}); apply->setEnabled(true); });
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    return dialog.exec();
}
