#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSaveFile>
#include <QSlider>
#include <QSpinBox>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTimer>
#include <QtTest/QTest>
#include <cmath>
#include <iostream>
#include "../appearance.h"

static double luminance(const QColor &color) {
    auto linear = [](double n) { return n <= .04045 ? n / 12.92 : std::pow((n + .055) / 1.055, 2.4); };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}
int main(int argc, char **argv) {
    QApplication app(argc, argv); app.addLibraryPath(app.applicationDirPath());
    auto style = QStyleFactory::create("SynterraGlass");
    if (!style) { std::cerr << "FAIL: Synterra Glass plugin could not load.\n"; return 1; }
    app.setStyle(style); GlassAppearance::syncPalette();
    const bool dark = GlassAppearance::dark();
    const double foreground = luminance(app.palette().color(QPalette::Text)), background = luminance(app.palette().color(QPalette::Base));
    if ((std::max(foreground, background) + .05) / (std::min(foreground, background) + .05) < 4.5) { std::cerr << "FAIL: text contrast.\n"; return 1; }
    QWidget window; window.setWindowTitle(QString("Synterra 1.1 Build 1105 · %1 Glass").arg(dark ? "Dark" : "Light")); window.resize(760, 540);
    auto layout = new QVBoxLayout(&window); layout->setContentsMargins(24, 24, 24, 24); layout->setSpacing(18);
    auto title = new QLabel(QString("<h1>Synterra Glass %1</h1><p>Original application controls · Build 1105</p>").arg(dark ? "Dark" : "Light")); layout->addWidget(title);
    auto grid = new QGridLayout; layout->addLayout(grid);
    auto button = new QPushButton("Glass button"), disabled = new QPushButton("Disabled button"); disabled->setEnabled(false); grid->addWidget(button, 0, 0); grid->addWidget(disabled, 0, 1);
    auto check = new QCheckBox("Keyboard and pointer checkboxes"); grid->addWidget(check, 1, 0);
    auto radio = new QRadioButton("Radio selection"); radio->setChecked(true); grid->addWidget(radio, 1, 1);
    auto edit = new QLineEdit("Readable input and focus outlines"); grid->addWidget(edit, 2, 0);
    auto combo = new QComboBox; combo->addItems({"Light glass", "Dark glass"}); combo->setCurrentIndex(dark ? 1 : 0); grid->addWidget(combo, 2, 1);
    auto spin = new QSpinBox; spin->setRange(0, 2000); spin->setValue(1105); grid->addWidget(spin, 3, 0);
    auto slider = new QSlider(Qt::Horizontal); slider->setValue(65); grid->addWidget(slider, 3, 1);
    auto tabs = new QTabWidget; for (const auto name : {"Overview", "Options", "Details"}) { auto page = new QWidget; auto row = new QVBoxLayout(page); row->addWidget(new QLabel("Glass surfaces, clear text and native keyboard navigation.")); tabs->addTab(page, name); } layout->addWidget(tabs);
    auto progress = new QProgressBar; progress->setValue(72); layout->addWidget(progress); window.show();
    QTimer::singleShot(300, &app, [&] {
        check->setFocus(); QTest::keyClick(check, Qt::Key_Space); QTest::mouseClick(button, Qt::LeftButton);
        if (!check->isChecked() || !spin->isEnabled() || !window.grab().save(qEnvironmentVariable("SYNTERRA_THEME_SCREENSHOT"))) { app.exit(1); return; }
        if (!app.arguments().contains("--watch-test")) { std::cout << "PASS: real Synterra Glass plugin, readable palette and keyboard/pointer controls.\n"; app.quit(); return; }
        GlassAppearance::watch(&window, [&] { if (!GlassAppearance::dark() || app.palette().color(QPalette::Window).lightness() >= 128) { app.exit(1); return; } std::cout << "PASS: live light-to-dark palette notification after atomic config replacement.\n"; app.quit(); });
        QSaveFile file(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals");
        if (!file.open(QIODevice::WriteOnly) || file.write("[General]\nColorScheme=SynterraGlassDark\n") < 0 || !file.commit()) app.exit(1);
    });
    QTimer::singleShot(5000, &app, [&] { std::cerr << "FAIL: preview timed out.\n"; app.exit(1); });
    return app.exec();
}
