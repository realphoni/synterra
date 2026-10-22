#include "release.h"
#include "../desktop/appearance.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <unistd.h>

class UpdateWindow : public QWidget {
    Updates::System system;
    Updates::Plan plan;
    QString cached;
    QLabel *status, *detail, *current;
    QTextEdit *notes;
    QProgressBar *progress;
    QPushButton *check, *cancelButton, *install, *later, *release;
    QThread *worker = nullptr;
    std::atomic_bool cancelled = false;
    bool busy = false, ready = false, preview;
    void style() {
        const bool dark = GlassAppearance::dark();
        setStyleSheet(QString("QWidget {font-size:14px;} QFrame#hero {background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2);border:1px solid #609a94;border-radius:16px;} QFrame#hero QLabel {color:#effffa;background:transparent;} QPushButton {padding:9px 15px;min-height:20px;} QLabel#status {font-size:22px;font-weight:600;} QTextEdit {border:1px solid %3;border-radius:9px;padding:8px;}")
            .arg(dark ? "#113d40" : "#216361", dark ? "#243f5f" : "#487a99", dark ? "#40536a" : "#b3cbd8"));
    }
    void finish(const QString &message, const QString &description, bool downloaded = false) {
        busy = false; ready = downloaded; status->setText(message); detail->setText(description);
        check->setEnabled(!preview); cancelButton->hide(); install->setVisible(ready && !system.live);
        later->setVisible(ready && !system.live); progress->setVisible(false);
    }
    void ask() {
        if (!ready || system.live || busy || preview) return;
        QMessageBox box(QMessageBox::Question, "Update Synterra?",
            QString("Synterra %1 Build %2 has been downloaded and verified.\n\nUpdate now? Your files, bookmarks and desktop preferences are retained. The update terminal will ask for your administrator password and show the package changes. Close your other applications first.").arg(plan.version.text).arg(plan.version.build), QMessageBox::NoButton, this);
        auto now = box.addButton("Update now", QMessageBox::AcceptRole); auto postpone = box.addButton("Later", QMessageBox::RejectRole); box.setDefaultButton(postpone); box.exec();
        if (box.clickedButton() == now) {
            if (!QProcess::startDetached("/usr/bin/konsole", {"--hold", "-e", "/usr/bin/sudo", "/usr/local/libexec/synterra-release-update", "--apply", plan.tag, cached}))
                QMessageBox::warning(this, "Could not open updater", "The update terminal could not be started. Your downloaded update is still available.");
            else detail->setText("Continue in the update terminal. Check again after it finishes to refresh the installed version.");
        }
    }
    void start() {
        if (busy || preview) return;
        try { system = Updates::installedSystem(); }
        catch (const std::exception &error) { finish("Updates unavailable", QString::fromUtf8(error.what())); return; }
        current->setText(QString("Installed · Synterra %1 · Build %2").arg(system.version.text).arg(system.version.build));
        busy = true; ready = false; cancelled = false; install->hide(); later->hide(); release->hide(); notes->clear();
        check->setEnabled(false); cancelButton->setEnabled(true); cancelButton->show(); progress->show(); progress->setRange(0, 0);
        status->setText("Checking for a new release…"); detail->setText("Connecting to Synterra's GitHub releases.");
        auto thread = QThread::create([this] {
            try {
                auto releaseData = Updates::latest(cancelled);
                const auto tag = releaseData.value("tag_name").toString();
                // Older releases did not ship an in-place update manifest.
                if (tag.startsWith('v') && Updates::Version::parse(tag.mid(1).section('.', 0, 1), 1).numbers < system.version.numbers) {
                    QMetaObject::invokeMethod(this, [this] { finish("You're up to date", "No newer full Synterra release is available."); }); return;
                }
                auto next = Updates::resolve(releaseData, system, cancelled);
                if (!(system.version < next.version)) {
                    QMetaObject::invokeMethod(this, [this] { finish("You're up to date", "You have the latest full Synterra release."); }); return;
                }
                QMetaObject::invokeMethod(this, [this, next] {
                    plan = next; release->show(); notes->setPlainText(next.notes);
                    status->setText(QString("Synterra %1 · Build %2").arg(next.version.text).arg(next.version.build));
                    detail->setText(system.live ? "A newer release is available. Install Synterra first to use release upgrades." : "Downloading your update. You'll be asked before anything is installed.");
                });
                if (system.live) { QMetaObject::invokeMethod(this, [this] { finish("New release available", "Release upgrades are available after installing Synterra. Open the release page for the live ISO."); }); return; }
                const auto directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + next.tag;
                auto path = Updates::stage(next.package, directory, cancelled, [this](qint64 done, qint64 total) {
                    QMetaObject::invokeMethod(this, [this, done, total] {
                        progress->setRange(0, 1000); progress->setValue(int(done * 1000 / qMax<qint64>(total, 1)));
                        detail->setText(QString("Downloading · %1 of %2 MiB").arg(done / 1048576.0, 0, 'f', 1).arg(total / 1048576.0, 0, 'f', 1));
                    });
                });
                Updates::inspectPackage(path, next);
                QMetaObject::invokeMethod(this, [this, next, path] {
                    plan = next; cached = path; finish("Your update is ready", "Downloaded and verified. Choose Update now, or keep it for later.", true); ask();
                });
            } catch (const std::exception &error) {
                const auto message = QString::fromUtf8(error.what());
                QMetaObject::invokeMethod(this, [this, message] { finish(cancelled ? "Download cancelled" : "Couldn't complete the update check", message); });
            }
        });
        worker = thread;
        connect(thread, &QThread::finished, this, [this, thread] { if (worker == thread) worker = nullptr; thread->deleteLater(); }); thread->start();
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        if (busy && QMessageBox::question(this, "Cancel download?", "Close Synterra Updates and cancel the current check or download?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) { event->ignore(); return; }
        cancelled = true; event->accept();
    }
public:
    explicit UpdateWindow(bool isPreview) : preview(isPreview) {
        setWindowTitle("Synterra Updates"); setWindowIcon(QIcon::fromTheme("synterra-updates", QIcon::fromTheme("system-software-update"))); resize(710, 620);
        auto layout = new QVBoxLayout(this); layout->setContentsMargins(24,24,24,22); layout->setSpacing(18);
        auto hero = new QFrame; hero->setObjectName("hero"); auto heroLayout = new QVBoxLayout(hero); heroLayout->setContentsMargins(22,18,22,18);
        auto title = new QLabel("<h1>Synterra Updates</h1>"); heroLayout->addWidget(title);
        heroLayout->addWidget(new QLabel("Variscite · Your next release, ready when you are.")); layout->addWidget(hero);
        current = new QLabel("Synterra 2.0 · Build 2010"); layout->addWidget(current);
        status = new QLabel("Stay in step with Synterra"); status->setObjectName("status"); status->setWordWrap(true); layout->addWidget(status);
        detail = new QLabel("Checks full releases on GitHub, downloads newer updates and asks before installing."); detail->setWordWrap(true); detail->setTextFormat(Qt::PlainText); layout->addWidget(detail);
        progress = new QProgressBar; progress->hide(); layout->addWidget(progress);
        notes = new QTextEdit; notes->setReadOnly(true); notes->setPlaceholderText("Release notes will appear here."); notes->setMinimumHeight(150); layout->addWidget(notes,1);
        auto actions = new QHBoxLayout; check = new QPushButton("Check for updates"); cancelButton = new QPushButton("Cancel download"); cancelButton->hide();
        release = new QPushButton("Release page"); release->hide(); later = new QPushButton("Later"); later->hide(); install = new QPushButton("Update now"); install->hide();
        for (auto button : {check,cancelButton,release,later,install}) actions->addWidget(button); layout->addLayout(actions);
        auto note = new QLabel("Checks and downloads run without administrator access. Installing uses the normal Arch package manager."); note->setWordWrap(true); layout->addWidget(note);
        connect(check,&QPushButton::clicked,this,[this]{start();}); connect(cancelButton,&QPushButton::clicked,this,[this]{cancelled=true;cancelButton->setEnabled(false);});
        connect(install,&QPushButton::clicked,this,[this]{ask();}); connect(later,&QPushButton::clicked,this,[this]{detail->setText("Saved for later. Choose Update now whenever you're ready.");});
        connect(release,&QPushButton::clicked,this,[this]{QDesktopServices::openUrl(plan.page);});
        style(); GlassAppearance::watch(this,[this]{style();});
        if (!preview) QTimer::singleShot(0,this,[this]{start();}); else check->setEnabled(false);
    }
    ~UpdateWindow() override { cancelled = true; if (worker) worker->wait(); }
};
int main(int argc, char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("Synterra"); app.setApplicationName("SynterraUpdates"); app.setApplicationDisplayName("Synterra Updates");
    GlassAppearance::syncPalette(); const bool preview = app.arguments().contains("--preview");
    if (geteuid() == 0 && !preview) { QMessageBox::warning(nullptr,"Synterra Updates","Open this app as your regular desktop account. Administrator access is requested only when you choose to install."); return 1; }
    UpdateWindow window(preview); window.show();
    const auto index = app.arguments().indexOf("--screenshot");
    if (preview && index > 0 && index + 1 < argc) QTimer::singleShot(400,&app,[&]{app.exit(window.grab().save(app.arguments().at(index+1)) ? 0 : 1);});
    return app.exec();
}
