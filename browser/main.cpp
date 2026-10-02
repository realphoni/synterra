#include <QApplication>
#include <QCoreApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QScrollArea>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineFindTextResult>
#include <QUrlQuery>
#include <QWebEngineDownloadRequest>
#include <QWebEngineHistory>
#include <QWebEngineNewWindowRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <algorithm>
#include <iostream>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QtTest/QTest>
#include "state.h"

static const QUrl homeUrl(QStringLiteral("qrc:/surf/start.html"));

static QUrl destination(QString input) {
    input = input.trimmed();
    if (input.isEmpty()) return homeUrl;
    if (input.startsWith("https://", Qt::CaseInsensitive) || input.startsWith("http://", Qt::CaseInsensitive)) {
        QUrl url(input);
        return url.isValid() && !url.host().isEmpty() ? url : QUrl();
    }
    if (input.contains("://")) return QUrl();
    if (!input.contains(' ') && (input.contains('.') || input == "localhost" || input.startsWith("localhost:") || input.startsWith('['))) {
        // A colon in the path is not a port. Validate the authority separately.
        const auto authority = input.section('/', 0, 0).section('?', 0, 0).section('#', 0, 0);
        if (authority.contains('@')) return QUrl();
        const QRegularExpression hostPort("^(?:\\[[0-9a-fA-F:]+\\]|[A-Za-z0-9.-]+)(?::([0-9]+))?$");
        const auto match = hostPort.match(authority);
        if (!match.hasMatch() || (!match.captured(1).isEmpty() && (match.captured(1).toInt() < 1 || match.captured(1).toInt() > 65535))) return QUrl();
        QUrl url("https://" + input);
        return url.isValid() && !url.host().isEmpty() ? url : QUrl();
    }
    if (input.contains(":") && !input.contains(' ')) return QUrl();
    QUrl result("https://duckduckgo.com/");
    QUrlQuery query;
    query.addQueryItem("q", input);
    result.setQuery(query);
    return result;
}

class Surf final : public QMainWindow {
    QTabWidget *tabs = new QTabWidget(this);
    QLineEdit *address = new QLineEdit(this);
    QWebEngineProfile *profile;
    bool privateMode;
    QSettings settings;
    QAction *backAction = nullptr, *forwardAction = nullptr;
    QWidget *findBar = new QWidget(this);
    QLineEdit *findInput = new QLineEdit(findBar);
    QLabel *findResult = new QLabel(findBar);
    QDialog *downloads = new QDialog(this);
    QVBoxLayout *downloadRows = new QVBoxLayout;
    QStringList closedTabs;
    int activeDownloads = 0;
    QString smokeDownloadPath;
    bool smokeDownloadSaved = false;

    void find(bool backwards = false) {
        if (current()) current()->findText(findInput->text(), backwards ? QWebEnginePage::FindBackward : QWebEnginePage::FindFlags());
    }
    void showFind() {
        findBar->show(); findInput->setFocus(); findInput->selectAll();
    }
    void saveSession() {
        if (privateMode || !settings.value("restoreSession", false).toBool()) return;
        QStringList urls;
        for (int i = 0; i < tabs->count() && urls.size() < 30; ++i) {
            auto url = qobject_cast<QWebEngineView *>(tabs->widget(i))->url();
            if (webAddress(url)) urls.append(withoutCredentials(url).toString());
        }
        settings.setValue("session", urls);
    }
    void history() {
        if (privateMode) return;
        QDialog dialog(this); dialog.setWindowTitle("Browsing history"); dialog.resize(650, 450);
        QVBoxLayout layout(&dialog); QLineEdit filter; filter.setPlaceholderText("Search history");
        QListWidget list; layout.addWidget(&filter); layout.addWidget(&list);
        for (const auto &value : settings.value("history").toList()) {
            const auto entry = value.toMap();
            auto item = new QListWidgetItem(entry.value("title").toString() + "\n" + entry.value("url").toString() + " · " + entry.value("time").toDateTime().toLocalTime().toString("MMM d, hh:mm"), &list);
            item->setData(Qt::UserRole, entry.value("url"));
        }
        QPushButton clear("Clear history"); layout.addWidget(&clear);
        connect(&filter, &QLineEdit::textChanged, &dialog, [&list](const QString &text) {
            for (int i = 0; i < list.count(); ++i) list.item(i)->setHidden(!list.item(i)->text().contains(text, Qt::CaseInsensitive));
        });
        connect(&list, &QListWidget::itemActivated, &dialog, [this, &dialog](QListWidgetItem *item) { addTab(QUrl(item->data(Qt::UserRole).toString())); dialog.accept(); });
        connect(&clear, &QPushButton::clicked, &dialog, [this, &dialog, &list] {
            if (QMessageBox::question(&dialog, "Clear history", "Remove Surf's saved browsing history?") != QMessageBox::Yes) return;
            settings.remove("history"); profile->clearAllVisitedLinks(); list.clear();
        });
        dialog.exec();
    }
    void preferences() {
        QDialog dialog(this); dialog.setWindowTitle("Surf settings"); QVBoxLayout layout(&dialog);
        QCheckBox restore("Restore my open web tabs when Surf starts"); restore.setChecked(settings.value("restoreSession", false).toBool());
        QCheckBox dark("Dark browser toolbar"); dark.setChecked(settings.value("darkToolbar", false).toBool());
        QLabel note("Private windows never save browsing history or sessions.\nDownloads and bookmarks you save are kept on disk."); note.setWordWrap(true);
        QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        layout.addWidget(&restore); layout.addWidget(&dark); layout.addWidget(&note); layout.addWidget(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        settings.setValue("restoreSession", restore.isChecked());
        if (!restore.isChecked()) settings.remove("session");
        settings.setValue("darkToolbar", dark.isChecked()); applyStyle(); saveSession();
    }
    void applyStyle() {
        const bool dark = privateMode || settings.value("darkToolbar", false).toBool();
        setStyleSheet(dark
            ? "QToolBar {background:#1d2940; color:#e6f0ff; padding:7px; spacing:6px;} QLineEdit {background:#273955; color:#f1f6ff; border:1px solid #5476a0; border-radius:10px; padding:8px 12px; min-width:180px;} QToolButton {color:#e6f0ff; padding:7px; border-radius:6px;} QToolButton:hover {background:#354b70;} QTabBar::tab {padding:9px 17px;}"
            : "QToolBar {background:#e7f0fa; padding:7px; spacing:6px; border-bottom:1px solid #bdcede;} QLineEdit {background:white; color:#19354e; border:1px solid #b6cee4; border-radius:10px; padding:8px 12px; min-width:180px;} QToolButton {padding:7px; border-radius:6px;} QToolButton:hover {background:#cfe4f6;} QTabBar::tab {padding:9px 17px;}");
    }
    void trackDownload(QWebEngineDownloadRequest *download, const QString &path) {
        download->setDownloadDirectory(QFileInfo(path).absolutePath());
        download->setDownloadFileName(QFileInfo(path).fileName());
        auto row = new QWidget(downloads); auto layout = new QVBoxLayout(row);
        auto label = new QLabel(QFileInfo(path).fileName(), row); label->setTextFormat(Qt::PlainText); layout->addWidget(label);
        auto progress = new QProgressBar(row); layout->addWidget(progress);
        auto controls = new QHBoxLayout; auto cancel = new QPushButton("Cancel", row);
        auto folder = new QPushButton("Show folder", row); controls->addWidget(cancel); controls->addWidget(folder); layout->addLayout(controls);
        downloadRows->insertWidget(downloadRows->count() - 1, row);
        QPointer<QWebEngineDownloadRequest> guarded(download);
        connect(cancel, &QPushButton::clicked, row, [guarded] { if (guarded) guarded->cancel(); });
        connect(folder, &QPushButton::clicked, row, [path] { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath())); });
        auto update = [guarded, label, progress, cancel] {
            if (!guarded) return;
            const auto total = guarded->totalBytes(), received = guarded->receivedBytes();
            progress->setRange(0, total > 0 ? 100 : 0);
            if (total > 0) progress->setValue(int(100.0 * received / total));
            QString state = QString::number(received / 1024) + " KiB received";
            switch (guarded->state()) {
                case QWebEngineDownloadRequest::DownloadCompleted: state = "Saved"; break;
                case QWebEngineDownloadRequest::DownloadCancelled: state = "Cancelled"; break;
                case QWebEngineDownloadRequest::DownloadInterrupted: state = "Interrupted: " + guarded->interruptReasonString(); break;
                default: break;
            }
            label->setText(guarded->downloadFileName() + " — " + state);
            cancel->setEnabled(!guarded->isFinished());
            if (guarded->isFinished() && total <= 0) { progress->setRange(0, 1); progress->setValue(1); }
        };
        connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, row, update);
        connect(download, &QWebEngineDownloadRequest::totalBytesChanged, row, update);
        connect(download, &QWebEngineDownloadRequest::stateChanged, row, update);
        connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, guarded] {
            if (guarded && guarded->state() == QWebEngineDownloadRequest::DownloadCompleted && !smokeDownloadPath.isEmpty()) {
                QFile file(smokeDownloadPath);
                smokeDownloadSaved = file.open(QIODevice::ReadOnly) && file.readAll() == "Synterra Surf download fixture\n";
                downloads->hide();
            }
        });
        ++activeDownloads;
        connect(download, &QWebEngineDownloadRequest::isFinishedChanged, this, [this, guarded] { if (guarded && guarded->isFinished()) --activeDownloads; });
        download->accept(); update(); downloads->show(); downloads->raise();
    }

    QWebEngineView *current() const { return qobject_cast<QWebEngineView *>(tabs->currentWidget()); }
    void syncNavigation() {
        auto view = current();
        if (!view) return;
        address->setText(view->url() == homeUrl ? QString() : view->url().toDisplayString());
        backAction->setEnabled(view->history()->canGoBack());
        forwardAction->setEnabled(view->history()->canGoForward());
        setWindowTitle((view->title().isEmpty() ? "New tab" : view->title()) + " — Synterra Surf" + (privateMode ? " · Private" : ""));
        statusBar()->showMessage(view->url().scheme() == "https" ? "HTTPS" : view->url() == homeUrl ? privateMode ? "Private window · downloads are saved to disk" : "Welcome to Synterra Surf" : view->url().scheme().toUpper());
    }
    void closeTab(int index) {
        QWidget *view = tabs->widget(index);
        if (!view) return;
        const auto url = qobject_cast<QWebEngineView *>(view)->url();
        if (webAddress(url)) { closedTabs.append(url.toString()); if (closedTabs.size() > 10) closedTabs.removeFirst(); }
        tabs->removeTab(index);
        delete view;
        if (!tabs->count()) addTab(homeUrl);
    }
    void navigate() {
        QUrl url = destination(address->text());
        if (!url.isValid() || url.isEmpty()) {
            statusBar()->showMessage("Enter a web address or search terms.", 5000);
            return;
        }
        current()->load(url);
    }
    void bookmarks() {
        QMenu menu(this);
        menu.addAction("Bookmark this page", this, [this] {
            if (!current() || !current()->url().scheme().startsWith("http")) return;
            auto saved = settings.value("bookmarks").toStringList();
            const QString url = withoutCredentials(current()->url()).toString();
            if (!saved.contains(url)) saved.append(url);
            settings.setValue("bookmarks", saved);
            statusBar()->showMessage("Bookmark saved", 2500);
        });
        menu.addSeparator();
        const auto saved = settings.value("bookmarks").toStringList();
        for (const auto &url : saved) menu.addAction(url, this, [this, url] { addTab(destination(url)); });
        if (saved.isEmpty()) menu.addAction("Your saved pages appear here")->setEnabled(false);
        else { menu.addSeparator(); menu.addAction("Clear bookmarks", this, [this] {
            if (QMessageBox::question(this, "Bookmarks", "Remove all saved bookmarks?") == QMessageBox::Yes) settings.remove("bookmarks");
        }); }
        if (current()) menu.addAction("Remove bookmark for this page", this, [this] {
            auto saved = settings.value("bookmarks").toStringList(); saved.removeAll(withoutCredentials(current()->url()).toString()); settings.setValue("bookmarks", saved);
        });
        menu.exec(QCursor::pos());
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        if (activeDownloads && QMessageBox::question(this, "Downloads in progress", "Close Surf and cancel active downloads?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) { event->ignore(); return; }
        saveSession(); QMainWindow::closeEvent(event);
    }
public:
    explicit Surf(bool privateWindow = false) : profile(privateWindow ? new QWebEngineProfile(this) : new QWebEngineProfile("SynterraSurf", this)), privateMode(privateWindow) {
        resize(1180, 790);
        setWindowIcon(QIcon::fromTheme("synterra-surf"));
        const auto data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!privateMode) {
            profile->setPersistentStoragePath(data + "/profile");
            profile->setCachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        }
        tabs->setDocumentMode(true);
        tabs->setTabsClosable(true);
        tabs->setMovable(true);
        auto central = new QWidget(this); auto vertical = new QVBoxLayout(central); vertical->setContentsMargins(0, 0, 0, 0);
        vertical->addWidget(tabs); vertical->addWidget(findBar); setCentralWidget(central);
        auto findLayout = new QHBoxLayout(findBar); findInput->setPlaceholderText("Find in page");
        auto previous = new QPushButton("Previous", findBar), next = new QPushButton("Next", findBar), done = new QPushButton("Done", findBar);
        findLayout->addWidget(findInput); findLayout->addWidget(findResult); findLayout->addWidget(previous); findLayout->addWidget(next); findLayout->addWidget(done); findBar->hide();
        connect(findInput, &QLineEdit::textChanged, this, [this] { find(); });
        connect(findInput, &QLineEdit::returnPressed, this, [this] { find(); });
        connect(previous, &QPushButton::clicked, this, [this] { find(true); });
        connect(next, &QPushButton::clicked, this, [this] { find(); });
        connect(done, &QPushButton::clicked, this, [this] { findBar->hide(); current()->findText({}); current()->setFocus(); });
        downloads->setWindowTitle("Surf downloads"); downloads->resize(540, 380);
        auto outer = new QVBoxLayout(downloads); auto scroll = new QScrollArea(downloads); scroll->setWidgetResizable(true);
        auto content = new QWidget(scroll); content->setLayout(downloadRows); scroll->setWidget(content); outer->addWidget(scroll);
        auto folderButton = new QPushButton("Open Downloads folder", downloads); outer->addWidget(folderButton);
        connect(folderButton, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))); });
        auto hint = new QLabel("Downloads saved in this window appear here.", content); hint->setWordWrap(true); downloadRows->addWidget(hint); downloadRows->addStretch();
        auto bar = addToolBar("Navigation");
        bar->setMovable(false);
        bar->setIconSize(QSize(20, 20));
        backAction = bar->addAction(QIcon::fromTheme("go-previous"), "Back", this, [this] { current()->back(); });
        forwardAction = bar->addAction(QIcon::fromTheme("go-next"), "Forward", this, [this] { current()->forward(); });
        bar->addAction(QIcon::fromTheme("view-refresh"), "Reload", this, [this] { current()->reload(); });
        bar->addAction(QIcon::fromTheme("go-home"), "Home", this, [this] { current()->load(homeUrl); });
        address->setPlaceholderText("Search with DuckDuckGo or enter a web address");
        address->setClearButtonEnabled(true);
        bar->addWidget(address);
        bar->addAction(QIcon::fromTheme("tab-new"), "New tab", this, [this] { addTab(homeUrl); });
        bar->addAction(QIcon::fromTheme("bookmark-new"), "Bookmarks", this, [this] { bookmarks(); });
        bar->addAction(QIcon::fromTheme("folder-download"), "Downloads", this, [this] { downloads->show(); downloads->raise(); });
        auto menuButton = new QToolButton(bar); menuButton->setText("Menu"); menuButton->setIcon(QIcon::fromTheme("application-menu")); menuButton->setPopupMode(QToolButton::InstantPopup);
        auto menu = new QMenu(menuButton); menuButton->setMenu(menu); bar->addWidget(menuButton);
        menu->addAction("New private window", this, [] { auto window = new Surf(true); window->setAttribute(Qt::WA_DeleteOnClose); window->addTab(homeUrl); window->show(); });
        menu->addAction("Find in page", this, [this] { showFind(); });
        menu->addAction("Browsing history", this, [this] { history(); })->setEnabled(!privateMode);
        menu->addAction("Reopen closed tab", this, [this] { reopen(); });
        menu->addAction("Reset page zoom", this, [this] { current()->setZoomFactor(1); });
        menu->addSeparator();
        menu->addAction("Settings", this, [this] { preferences(); })->setEnabled(!privateMode);
        menu->addAction("About Surf", this, [this] {
            QMessageBox::about(this, "Synterra Surf", "<h2>Synterra Surf</h2><p>Indev · Prism 0.4</p><p>Powered by Qt WebEngine. Private windows use an isolated memory profile; downloaded files and saved bookmarks remain on disk.</p><p>Ctrl+L address · Ctrl+T new tab · Ctrl+D bookmark<br>Ctrl+F find · Ctrl+H history · Ctrl+J downloads<br>Ctrl+Shift+T reopen · Ctrl+Shift+N private window<br>Ctrl+W close · Ctrl+R reload · Ctrl++ / Ctrl+- zoom</p>");
        });
        applyStyle();
        connect(address, &QLineEdit::returnPressed, this, [this] { navigate(); });
        connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int index) { closeTab(index); });
        connect(tabs, &QTabWidget::currentChanged, this, [this] { syncNavigation(); findResult->clear(); if (findBar->isVisible()) find(); });
        auto shortcut = [this](const QString &key, auto action) { connect(new QShortcut(QKeySequence(key), this), &QShortcut::activated, this, action); };
        shortcut("Ctrl+L", [this] { address->setFocus(); address->selectAll(); });
        shortcut("Ctrl+T", [this] { addTab(homeUrl); address->setFocus(); });
        shortcut("Ctrl+W", [this] { closeTab(tabs->currentIndex()); });
        shortcut("Ctrl+R", [this] { current()->reload(); });
        shortcut("Ctrl+D", [this] { bookmarks(); });
        shortcut("Ctrl+F", [this] { showFind(); });
        shortcut("Ctrl+H", [this] { history(); });
        shortcut("Ctrl+J", [this] { downloads->show(); downloads->raise(); });
        shortcut("Ctrl+Shift+T", [this] { reopen(); });
        shortcut("Ctrl+Shift+N", [] { auto window = new Surf(true); window->setAttribute(Qt::WA_DeleteOnClose); window->addTab(homeUrl); window->show(); });
        shortcut("Ctrl+0", [this] { current()->setZoomFactor(1); });
        shortcut("Escape", [this] { if (findBar->isVisible()) { findBar->hide(); current()->findText({}); current()->setFocus(); } else current()->stop(); });
        shortcut("Alt+Left", [this] { current()->back(); });
        shortcut("Alt+Right", [this] { current()->forward(); });
        shortcut("Ctrl++", [this] { current()->setZoomFactor(std::min(3.0, current()->zoomFactor() + .1)); });
        shortcut("Ctrl+-", [this] { current()->setZoomFactor(std::max(.5, current()->zoomFactor() - .1)); });
        connect(profile, &QWebEngineProfile::downloadRequested, this, [this](QWebEngineDownloadRequest *download) {
            const QString proposed = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).filePath(download->suggestedFileName());
            const QString path = smokeDownloadPath.isEmpty() ? QFileDialog::getSaveFileName(this, "Save download", proposed) : smokeDownloadPath;
            if (path.isEmpty()) { download->cancel(); return; }
            trackDownload(download, path);
        });
    }
    ~Surf() override {
        // Pages must be destroyed before the shared persistent profile.
        while (tabs->count()) { auto view = tabs->widget(0); tabs->removeTab(0); delete view; }
        delete profile;
    }
    void reopen() { if (!closedTabs.isEmpty()) addTab(QUrl(closedTabs.takeLast())); }
    QStringList startupTabs() { return savedTabs(settings, privateMode); }
    bool smokeWidgets() {
        activateWindow(); current()->setFocus(); QTest::qWait(100); QTest::keyClick(current(), Qt::Key_F, Qt::ControlModifier); findInput->setText("Synterra");
        if (!findBar->isVisible()) { std::cerr << "FAIL: Ctrl+F did not reveal find bar.\n"; return false; }
        const int count = tabs->count(); addTab(homeUrl); closeTab(tabs->currentIndex());
        if (tabs->count() != count) { std::cerr << "FAIL: tab closure.\n"; return false; }
        Surf privateWindow(true);
        if (!privateWindow.profile->isOffTheRecord() || privateWindow.profile->persistentCookiesPolicy() != QWebEngineProfile::NoPersistentCookies || !privateWindow.startupTabs().isEmpty()) { std::cerr << "FAIL: private profile isolation.\n"; return false; }
        return true;
    }
    QString findSummary() const { return findResult->text(); }
    void smokeDownload(const QUrl &url, const QString &path) { smokeDownloadPath = path; current()->page()->download(url); }
    bool downloadTestPassed() const { return smokeDownloadSaved && activeDownloads == 0; }
    QWebEngineView *addTab(const QUrl &url) {
        auto view = new QWebEngineView;
        auto page = new QWebEnginePage(profile, view);
        view->setPage(page);
        page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
        page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        connect(page, &QWebEnginePage::newWindowRequested, this, [this](QWebEngineNewWindowRequest &request) {
            if (request.isUserInitiated()) request.openIn(addTab(homeUrl)->page());
        });
        connect(page, &QWebEnginePage::permissionRequested, this, [this](QWebEnginePermission permission) {
            const auto answer = QMessageBox::question(this, "Website permission", permission.origin().toDisplayString() + " requests " + QVariant::fromValue(permission.permissionType()).toString() + ". Allow?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) permission.grant(); else permission.deny();
        });
        connect(page, &QWebEnginePage::findTextFinished, this, [this, view](const QWebEngineFindTextResult &result) {
            if (view == current()) findResult->setText(QString("%1 / %2").arg(result.activeMatch()).arg(result.numberOfMatches()));
        });
        tabs->addTab(view, "New tab");
        tabs->setCurrentWidget(view);
        connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
            tabs->setTabText(tabs->indexOf(view), title.isEmpty() ? "New tab" : title.left(28));
            if (view == current()) syncNavigation();
        });
        connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) { tabs->setTabIcon(tabs->indexOf(view), icon); });
        connect(view, &QWebEngineView::urlChanged, this, [this, view] { if (view == current()) syncNavigation(); });
        connect(view, &QWebEngineView::loadProgress, this, [this, view](int progress) { if (view == current() && progress < 100) statusBar()->showMessage(QString("Loading %1% …").arg(progress)); });
        connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
            if (ok) recordVisit(settings, privateMode, view->url(), view->title());
            if (view == current()) { syncNavigation(); if (!ok) statusBar()->showMessage("Page could not be loaded. Check the address and connection."); }
        });
        view->load(url.isValid() ? url : homeUrl);
        return view;
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QString(argv[1]) == "--self-test") {
        QCoreApplication app(argc, argv);
        if (destination("example.com") != QUrl("https://example.com") || destination("localhost:8443").port() != 8443 || destination("https://example.org/a").scheme() != "https" || QUrlQuery(destination("glass & prism")).queryItemValue("q") != "glass & prism" || destination("javascript:alert(1)").isValid() || destination("file:///etc/passwd").isValid() || destination("file:///home/example.txt").isValid()) return 1;
        if (destination("localhost:8443/a?x=1").port() != 8443 || destination("example.org/a:b").path() != "/a:b" || destination("example.org:99999/path").isValid() || destination("user:pass@example.org").isValid() || destination("[::1]:8443/a").port() != 8443) return 1;
        QTemporaryDir directory; if (!directory.isValid()) return 1;
        QSettings test(directory.filePath("state.ini"), QSettings::IniFormat);
        recordVisit(test, true, QUrl("https://private.example"), "Private");
        recordVisit(test, false, homeUrl, "Home");
        if (!test.value("history").toList().isEmpty()) return 1;
        for (int i = 0; i < 210; ++i) recordVisit(test, false, QUrl(QString("https://example.org/%1").arg(i)), "Example");
        recordVisit(test, false, QUrl("https://user:secret@example.org/209"), "Latest");
        auto visits = test.value("history").toList();
        if (visits.size() != 200 || visits.first().toMap().value("title").toString() != "Latest" || visits.first().toMap().value("url").toString().contains("secret")) return 1;
        test.setValue("session", QStringList{"https://example.org", "javascript:alert(1)", "file:///etc/passwd"});
        if (!savedTabs(test, false).isEmpty()) return 1;
        test.setValue("restoreSession", true);
        if (savedTabs(test, false).size() != 1 || !savedTabs(test, true).isEmpty()) return 1;
        test.sync(); QSettings reread(directory.filePath("state.ini"), QSettings::IniFormat);
        if (reread.value("history").toList().size() != 200) return 1;
        std::cout << "PASS: URL ports/paths, scheme rejection, history cap/deduplication, credential stripping and private/session isolation.\n";
        return 0;
    }
    QApplication app(argc, argv);
    app.setOrganizationName("Synterra");
    app.setApplicationName("SynterraSurf");
    app.setApplicationDisplayName("Synterra Surf");
    Surf window(app.arguments().contains("--private"));
    const bool smoke = app.arguments().contains("--smoke-test");
    QTemporaryDir downloadDirectory;
    QTcpServer fixture;
    if (smoke) {
        if (!downloadDirectory.isValid() || !fixture.listen(QHostAddress::LocalHost)) return 1;
        QObject::connect(&fixture, &QTcpServer::newConnection, &app, [&fixture] {
            auto socket = fixture.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("Synterra Surf download fixture\n");
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=fixture.txt\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }
    QStringList urls;
    if (!smoke) for (int i = 1; i < argc; ++i) if (!QString(argv[i]).startsWith("--")) { auto url = destination(QString::fromLocal8Bit(argv[i])); if (webAddress(url)) urls.append(url.toString()); }
    if (urls.isEmpty() && !smoke) urls = window.startupTabs();
    auto first = window.addTab(urls.isEmpty() ? homeUrl : QUrl(urls.takeFirst()));
    for (const auto &url : urls) window.addTab(QUrl(url));
    if (smoke) {
        QTimer::singleShot(20000, &app, [&app] { app.exit(1); });
        QObject::connect(first, &QWebEngineView::loadFinished, &app, [first, &window, &app, &fixture, &downloadDirectory](bool ok) {
            if (!ok) { app.exit(1); return; }
            first->page()->runJavaScript("document.title === 'Synterra Surf' && !!document.querySelector('#search')", [&window, &app, &fixture, &downloadDirectory](const QVariant &result) {
                if (!result.toBool()) { app.exit(1); return; }
                if (!window.smokeWidgets()) { app.exit(1); return; }
                window.smokeDownload(QUrl(QString("http://127.0.0.1:%1/download").arg(fixture.serverPort())), downloadDirectory.filePath("fixture.txt"));
                QTimer::singleShot(2500, &app, [&window, &app] {
                    if (window.findSummary().isEmpty() || window.findSummary().endsWith(" / 0")) { std::cerr << "FAIL: find results: " << window.findSummary().toStdString() << "\n"; app.exit(1); return; }
                    if (!window.downloadTestPassed()) { std::cerr << "FAIL: local HTTP download/progress completion.\n"; app.exit(1); return; }
                    const auto path = qEnvironmentVariable("SYNTERRA_SURF_SCREENSHOT");
                    if (!path.isEmpty() && !window.grab().save(path)) { app.exit(1); return; }
                    std::cout << "PASS: Chromium start page, Ctrl+F/find matches, tab closure, private profile and local HTTP download contents/completion.\n"; app.exit(0);
                });
            });
        });
    }
    window.show();
    return app.exec();
}
