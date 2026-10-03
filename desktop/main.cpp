#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtTest/QTest>
#include <iostream>

class Hub final : public QWidget {
    QSettings settings;
    bool live;
    QCheckBox *startup = new QCheckBox("Show this welcome when I sign in", this);
    QPushButton *installButton = nullptr, *updatesButton = nullptr;

    void launch(const QString &program, const QStringList &arguments = {}) {
        if (!QProcess::startDetached(program, arguments))
            QMessageBox::warning(this, "Could not open application", "The application could not be started. Check that it is installed.");
    }
    QPushButton *card(QGridLayout *grid, int index, const QString &title,
                      const QString &description, const QString &icon,
                      const QString &program, const QStringList &arguments = {}) {
        auto button = new QPushButton(QIcon::fromTheme(icon), title + "\n" + description, this);
        button->setMinimumHeight(76); button->setIconSize(QSize(28, 28));
        grid->addWidget(button, index / 2, index % 2);
        connect(button, &QPushButton::clicked, this, [this, program, arguments] { launch(program, arguments); });
        return button;
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        settings.setValue("seenVersion", "1.0");
        settings.setValue("showOnLogin", startup->isChecked());
        QWidget::closeEvent(event);
    }
public:
    explicit Hub(bool isLive) : live(isLive) {
        setWindowTitle("Synterra Hub"); setWindowIcon(QIcon(":/hub/synterra.svg")); resize(780, 650);
        setStyleSheet("QWidget {font-size:14px; color:#243950; background:#eef5fc;}"
                      "QWidget#hero {background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #25456c,stop:0.6 #476f9e,stop:1 #7273a4); border:1px solid #a7cce7; border-radius:14px;}"
                      "QWidget#hero QLabel {background:transparent; color:#f4faff;}"
                      "QPushButton {text-align:left; padding:12px; background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #ffffff,stop:1 #dfeefa); border:1px solid #a9c5dd; border-radius:9px;}"
                      "QPushButton:hover {background:#d5eafd; border-color:#568bc0;} QPushButton:disabled {color:#7d8b9a; background:#e6ecf2;}"
                      "QTabWidget::pane {border:0;} QTabBar::tab {padding:10px 18px;} QGroupBox {border:0;}");
        auto layout = new QVBoxLayout(this); layout->setContentsMargins(22, 22, 22, 18); layout->setSpacing(16);
        auto hero = new QWidget(this); hero->setObjectName("hero"); auto heroRow = new QHBoxLayout(hero);
        auto orb = new QLabel(hero); orb->setPixmap(QIcon(":/hub/synterra.svg").pixmap(72, 72)); heroRow->addWidget(orb);
        auto heading = new QLabel("<h1>Synterra 1.0</h1><p>Prism · Your desktop, brought together.</p>", hero); heroRow->addWidget(heading, 1); layout->addWidget(hero);
        auto tabs = new QTabWidget(this); layout->addWidget(tabs, 1);
        auto setup = new QWidget(tabs); auto setupLayout = new QVBoxLayout(setup);
        auto mode = new QLabel(live ? "Explore the live desktop, or install Synterra on your own disk." : "Welcome home. Set up your desktop and keep it up to date.", setup);
        mode->setWordWrap(true); setupLayout->addWidget(mode);
        auto grid = new QGridLayout; grid->setSpacing(10); setupLayout->addLayout(grid);
        card(grid, 0, "Desktop && appearance", "Make Prism your own", "preferences-desktop-theme", "systemsettings");
        card(grid, 1, "Displays", "Resolution and scaling", "preferences-desktop-display", "systemsettings", {"kcm_kscreen"});
        card(grid, 2, "Network", "Wi-Fi and connections", "network-wireless", "systemsettings", {"kcm_networkmanagement"});
        card(grid, 3, "Bluetooth", "Headphones and devices", "preferences-system-bluetooth", "systemsettings", {"kcm_bluetooth"});
        card(grid, 4, "Phone integration", "Pair with KDE Connect", "smartphone", "kdeconnect-app");
        updatesButton = new QPushButton(QIcon::fromTheme("system-software-update"), "System updates\nReview updates in the terminal", this);
        updatesButton->setMinimumHeight(76); updatesButton->setEnabled(!live); grid->addWidget(updatesButton, 2, 1);
        connect(updatesButton, &QPushButton::clicked, this, [this] {
            if (live) return;
            if (QMessageBox::question(this, "Update Synterra", "Open the system updater? It will ask for your administrator password and show the package changes before you confirm.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
                launch("konsole", {"--hold", "-e", "sudo", "pacman", "-Syu"});
        });
        if (live) {
            installButton = new QPushButton(QIcon::fromTheme("system-software-install"), "Install Synterra 1.0", setup);
            setupLayout->addWidget(installButton);
            connect(installButton, &QPushButton::clicked, this, [this] { launch("synterra-install"); });
        }
        auto note = new QLabel(live ? "System updates are available after installation. Connect to the internet before starting the guided installer." : "Updates use Arch's official repositories. Keep the system fully updated.", setup);
        note->setWordWrap(true); setupLayout->addWidget(note); setupLayout->addStretch(); tabs->addTab(setup, "Get started");
        auto apps = new QWidget(tabs); auto appLayout = new QVBoxLayout(apps); auto appGrid = new QGridLayout; appGrid->setSpacing(10); appLayout->addLayout(appGrid);
        card(appGrid, 0, "Synterra Surf", "Your browser", "synterra-surf", "synterra-surf");
        card(appGrid, 1, "Files", "Folders and archives", "system-file-manager", "dolphin");
        card(appGrid, 2, "Calculator", "Everyday and scientific maths", "accessories-calculator", "kcalc");
        card(appGrid, 3, "Screenshot", "Capture your desktop", "spectacle", "spectacle");
        card(appGrid, 4, "PDF reader", "Read your documents", "okular", "okular");
        card(appGrid, 5, "Media player", "Music and video", "vlc", "vlc");
        appLayout->addStretch(); tabs->addTab(apps, "Applications");
        auto about = new QWidget(tabs); auto aboutLayout = new QVBoxLayout(about);
        auto system = new QLabel("Synterra 1.0 (Prism)\nArch-based · KDE Plasma\n\nHost: " + QSysInfo::machineHostName() + "\nArchitecture: " + QSysInfo::currentCpuArchitecture() + "\nKernel: " + QSysInfo::kernelVersion(), about);
        system->setTextFormat(Qt::PlainText); system->setTextInteractionFlags(Qt::TextSelectableByMouse); aboutLayout->addWidget(system);
        auto help = new QPushButton("Synterra documentation", about); aboutLayout->addWidget(help);
        connect(help, &QPushButton::clicked, this, [this] { launch("synterra-surf", {"https://github.com/realphoni/synterra#readme"}); });
        auto arch = new QPushButton("Arch Wiki", about); aboutLayout->addWidget(arch);
        connect(arch, &QPushButton::clicked, this, [this] { launch("synterra-surf", {"https://wiki.archlinux.org/"}); });
        aboutLayout->addStretch(); tabs->addTab(about, "About && help");
        startup->setChecked(settings.value("showOnLogin", false).toBool()); layout->addWidget(startup);
    }
    bool smokeCheck() const { return !QIcon(":/hub/synterra.svg").pixmap(72, 72).isNull() && updatesButton->isEnabled() == !live && bool(installButton) == live; }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv); app.setOrganizationName("Synterra"); app.setApplicationName("SynterraHub"); app.setApplicationDisplayName("Synterra Hub");
    QSettings settings;
    if (app.arguments().contains("--welcome") && settings.value("seenVersion").toString() == "1.0" && !settings.value("showOnLogin", false).toBool()) return 0;
    Hub window(QDir("/run/archiso").exists()); window.show();
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(250, &app, [&app, &window] {
            Hub live(true), installed(false);
            if (!live.smokeCheck() || !installed.smokeCheck()) { app.exit(1); return; }
            const auto path = qEnvironmentVariable("SYNTERRA_HUB_SCREENSHOT");
            if (!path.isEmpty() && !window.grab().save(path)) { app.exit(1); return; }
            std::cout << "PASS: native Hub rendering and live/installed action guards.\n"; app.exit(0);
        });
    }
    return app.exec();
}
