#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
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

static const QUrl homeUrl(QStringLiteral("qrc:/surf/start.html"));

static QUrl destination(QString input) {
    input = input.trimmed();
    if (input.isEmpty()) return homeUrl;
    if (input.startsWith("https://", Qt::CaseInsensitive) || input.startsWith("http://", Qt::CaseInsensitive)) {
        QUrl url(input);
        return url.isValid() && !url.host().isEmpty() ? url : QUrl();
    }
    if (input.contains("://")) return QUrl();
    if (input.contains(':') && !input.contains(' ')) {
        bool validPort = false;
        const int port = input.section(':', 1).toInt(&validPort);
        if (!validPort || port < 1 || port > 65535) return QUrl();
    }
    if (!input.contains(' ') && (input.contains('.') || input == "localhost" || input.startsWith("localhost:"))) {
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
    QWebEngineProfile *profile = new QWebEngineProfile("SynterraSurf", this);
    QSettings settings;
    QAction *backAction = nullptr, *forwardAction = nullptr;

    QWebEngineView *current() const { return qobject_cast<QWebEngineView *>(tabs->currentWidget()); }
    void syncNavigation() {
        auto view = current();
        if (!view) return;
        address->setText(view->url() == homeUrl ? QString() : view->url().toDisplayString());
        backAction->setEnabled(view->history()->canGoBack());
        forwardAction->setEnabled(view->history()->canGoForward());
        setWindowTitle((view->title().isEmpty() ? "New tab" : view->title()) + " — Synterra Surf");
        statusBar()->showMessage(view->url().scheme() == "https" ? "HTTPS" : view->url() == homeUrl ? "Welcome to Synterra Surf" : view->url().scheme().toUpper());
    }
    void closeTab(int index) {
        QWidget *view = tabs->widget(index);
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
            const QString url = current()->url().toString();
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
        menu.exec(QCursor::pos());
    }
public:
    Surf() {
        resize(1180, 790);
        setWindowIcon(QIcon::fromTheme("synterra-surf"));
        const auto data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        profile->setPersistentStoragePath(data + "/profile");
        profile->setCachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        tabs->setDocumentMode(true);
        tabs->setTabsClosable(true);
        tabs->setMovable(true);
        setCentralWidget(tabs);
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
        bar->addAction(QIcon::fromTheme("folder-download"), "Downloads", this, [] {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
        });
        bar->addAction(QIcon::fromTheme("help-about"), "About Surf", this, [this] {
            QMessageBox::about(this, "Synterra Surf", "<h2>Synterra Surf</h2><p>Indev · Prism 0.3</p><p>A native browser powered by Qt WebEngine.</p><p>Ctrl+L address · Ctrl+T new tab · Ctrl+D bookmark<br>Ctrl+W close tab · Ctrl+R reload · Ctrl++ / Ctrl+- zoom</p>");
        });
        setStyleSheet("QToolBar {background:#e7f0fa; padding:7px; spacing:6px; border-bottom:1px solid #bdcede;} QLineEdit {background:white; color:#19354e; border:1px solid #b6cee4; border-radius:10px; padding:8px 12px; min-width:280px;} QToolButton {padding:7px; border-radius:6px;} QToolButton:hover {background:#cfe4f6;} QTabBar::tab {padding:9px 17px;}");
        connect(address, &QLineEdit::returnPressed, this, [this] { navigate(); });
        connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int index) { closeTab(index); });
        connect(tabs, &QTabWidget::currentChanged, this, [this] { syncNavigation(); });
        auto shortcut = [this](const QString &key, auto action) { connect(new QShortcut(QKeySequence(key), this), &QShortcut::activated, this, action); };
        shortcut("Ctrl+L", [this] { address->setFocus(); address->selectAll(); });
        shortcut("Ctrl+T", [this] { addTab(homeUrl); address->setFocus(); });
        shortcut("Ctrl+W", [this] { closeTab(tabs->currentIndex()); });
        shortcut("Ctrl+R", [this] { current()->reload(); });
        shortcut("Ctrl+D", [this] { bookmarks(); });
        shortcut("Alt+Left", [this] { current()->back(); });
        shortcut("Alt+Right", [this] { current()->forward(); });
        shortcut("Ctrl++", [this] { current()->setZoomFactor(std::min(3.0, current()->zoomFactor() + .1)); });
        shortcut("Ctrl+-", [this] { current()->setZoomFactor(std::max(.5, current()->zoomFactor() - .1)); });
        connect(profile, &QWebEngineProfile::downloadRequested, this, [this](QWebEngineDownloadRequest *download) {
            const QString proposed = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).filePath(download->suggestedFileName());
            const QString path = QFileDialog::getSaveFileName(this, "Save download", proposed);
            if (path.isEmpty()) { download->cancel(); return; }
            download->setDownloadDirectory(QFileInfo(path).absolutePath());
            download->setDownloadFileName(QFileInfo(path).fileName());
            connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, download] {
                if (download->state() == QWebEngineDownloadRequest::DownloadCompleted) statusBar()->showMessage("Download saved: " + download->downloadFileName(), 10000);
                if (download->state() == QWebEngineDownloadRequest::DownloadInterrupted) statusBar()->showMessage("Download interrupted: " + download->interruptReasonString(), 10000);
            });
            download->accept();
            statusBar()->showMessage("Downloading " + download->downloadFileName());
        });
    }
    ~Surf() override {
        // Pages must be destroyed before the shared persistent profile.
        while (tabs->count()) { auto view = tabs->widget(0); tabs->removeTab(0); delete view; }
        delete profile;
    }
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
        tabs->addTab(view, "New tab");
        tabs->setCurrentWidget(view);
        connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
            tabs->setTabText(tabs->indexOf(view), title.isEmpty() ? "New tab" : title.left(28));
            if (view == current()) syncNavigation();
        });
        connect(view, &QWebEngineView::urlChanged, this, [this, view] { if (view == current()) syncNavigation(); });
        connect(view, &QWebEngineView::loadProgress, this, [this, view](int progress) { if (view == current() && progress < 100) statusBar()->showMessage(QString("Loading %1% …").arg(progress)); });
        connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) { if (view == current()) { syncNavigation(); if (!ok) statusBar()->showMessage("Page could not be loaded. Check the address and connection."); } });
        view->load(url.isValid() ? url : homeUrl);
        return view;
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QString(argv[1]) == "--self-test") {
        QCoreApplication app(argc, argv);
        if (destination("example.com") != QUrl("https://example.com") || destination("localhost:8443").port() != 8443 || destination("https://example.org/a").scheme() != "https" || QUrlQuery(destination("glass & prism")).queryItemValue("q") != "glass & prism" || destination("javascript:alert(1)").isValid() || destination("file:///etc/passwd").isValid() || destination("file:///home/example.txt").isValid()) return 1;
        std::cout << "PASS: address normalization, search encoding and unsupported-scheme rejection.\n";
        return 0;
    }
    QApplication app(argc, argv);
    app.setOrganizationName("Synterra");
    app.setApplicationName("SynterraSurf");
    app.setApplicationDisplayName("Synterra Surf");
    Surf window;
    const bool smoke = app.arguments().contains("--smoke-test");
    auto first = window.addTab(homeUrl);
    if (!smoke) for (int i = 1; i < argc; ++i) { auto url = destination(QString::fromLocal8Bit(argv[i])); if (url.isValid()) window.addTab(url); }
    if (smoke) {
        QTimer::singleShot(20000, &app, [&app] { app.exit(1); });
        QObject::connect(first, &QWebEngineView::loadFinished, &app, [first, &window, &app](bool ok) {
            if (!ok) { app.exit(1); return; }
            first->page()->runJavaScript("document.title === 'Synterra Surf' && !!document.querySelector('#search')", [&window, &app](const QVariant &result) {
                if (!result.toBool()) { app.exit(1); return; }
                QTimer::singleShot(600, &app, [&window, &app] {
                    const auto path = qEnvironmentVariable("SYNTERRA_SURF_SCREENSHOT");
                    if (!path.isEmpty() && !window.grab().save(path)) { app.exit(1); return; }
                    std::cout << "PASS: browser engine rendered the start page.\n"; app.exit(0);
                });
            });
        });
    }
    window.show();
    return app.exec();
}
