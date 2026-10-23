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
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QScrollArea>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
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
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <algorithm>
#include <iostream>
#include <memory>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QSaveFile>
#include <QtTest/QTest>
#include "state.h"
#include "notices.h"
#include "../desktop/appearance.h"

static const QUrl homeUrl(QStringLiteral("qrc:/surf/start.html"));

// Automated fixtures may use a real-looking base URL for the address bar.
// Keep every test request, including automatic favicons, off the public web.
class FixtureRequests final : public QWebEngineUrlRequestInterceptor {
public:
    explicit FixtureRequests(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl();
        if ((url.scheme() == "https" || url.scheme() == "http") && url.host() != "127.0.0.1") info.block(true);
    }
};

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
    QAction *backAction = nullptr, *forwardAction = nullptr, *reloadAction = nullptr, *muteAction = nullptr;
    QProgressBar *pageProgress = new QProgressBar(this);
    QWidget *noticeBar = new QWidget(this);
    QLabel *noticeText = new QLabel(noticeBar);
    QWidget *topicBar = new QWidget(this);
    QLabel *topicIcon = new QLabel(topicBar), *topicHeading = new QLabel(topicBar), *topicText = new QLabel(topicBar);
    QPushButton *topicSource = new QPushButton("Read more", topicBar), *topicNext = new QPushButton(topicBar), *topicDismiss = new QPushButton("Dismiss", topicBar);
    QHash<QWebEngineView *, SurfNotices::TabState> topicStates;
    QHash<QWebEngineView *, QHash<QString, quint32>> topicVariants;
    bool noticesEnabled = true, searchNotices = true, playfulNotices = true;
    QUrl currentNoticeSource;
    QHash<QWebEngineView *, QPointer<QDialog>> inspectors;
    QWidget *findBar = new QWidget(this);
    QLineEdit *findInput = new QLineEdit(findBar);
    QLabel *findResult = new QLabel(findBar);
    QDialog *downloads = new QDialog(this);
    QVBoxLayout *downloadRows = new QVBoxLayout;
    QStringList closedTabs;
    int activeDownloads = 0;
    QString smokeDownloadPath;
    bool smokeDownloadSaved = false;
    QString smokePdfPath;
    bool smokePdfSaved = false;
    QLabel *zoomLabel = new QLabel("100%", this);
    Qt::WindowStates normalState;

    void reloadOrStop() {
        if (!current()) return;
        if (current()->page()->isLoading()) {
            current()->setProperty("surfStopped", true);
            current()->stop();
        } else current()->reload();
    }
    void syncTopicNotice() {
        auto view = current();
        currentNoticeSource.clear();
        if (!view || !noticesEnabled) { topicBar->hide(); return; }
        auto &state = topicStates[view];
        const auto context = view->url().adjusted(QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString();
        if (state.context != context) { state.context = context; state.selected = 0; }
        const auto matches = SurfNotices::visible(SurfNotices::match(view->url(), searchNotices, playfulNotices), state);
        if (matches.isEmpty()) { topicBar->hide(); return; }
        state.selected %= matches.size();
        const auto &entry = matches[state.selected];
        const auto &notice = entry.notice;
        auto &variants = topicVariants[view];
        if (!variants.contains(notice.id)) variants.insert(notice.id, QRandomGenerator::global()->generate());
        const QString kind = notice.warning ? "Warning" : "Information";
        topicHeading->setText(kind + " · " + notice.title + (entry.searchTopic ? " · Search topic" : " · This site"));
        topicText->setText(SurfNotices::message(notice, variants.value(notice.id)));
        topicBar->setProperty("tone", notice.warning ? "warning" : "information");
        topicIcon->setText(notice.warning ? QString::fromUtf8("⚠") : QString::fromUtf8("ⓘ"));
        topicIcon->setAccessibleName(kind);
        currentNoticeSource = notice.source;
        topicSource->setVisible(!currentNoticeSource.isEmpty());
        topicSource->setToolTip(currentNoticeSource.toDisplayString());
        topicNext->setVisible(matches.size() > 1);
        topicNext->setText(QString("Next · %1/%2").arg(state.selected + 1).arg(matches.size()));
        topicBar->style()->unpolish(topicBar); topicBar->style()->polish(topicBar);
        topicBar->show();
    }
    void dismissTopicNotice() {
        auto view = current();
        if (!view) return;
        auto &state = topicStates[view];
        const auto matches = SurfNotices::visible(SurfNotices::match(view->url(), searchNotices, playfulNotices), state);
        if (!matches.isEmpty()) state.dismissed.insert(matches[state.selected % matches.size()].notice.id);
        state.selected = 0; syncTopicNotice();
    }
    void updateTab(QWebEngineView *view) {
        const int index = tabs->indexOf(view);
        if (index < 0) return;
        const bool muted = view->page()->isAudioMuted();
        const QString title = view->title().isEmpty() ? "New tab" : view->title();
        tabs->setTabText(index, title.left(28));
        tabs->setTabToolTip(index, title + (muted ? " · Muted" : ""));
        tabs->setTabIcon(index, muted ? QIcon::fromTheme("audio-volume-muted") : view->icon());
    }
    void developerTools() {
        auto view = current();
        if (!view) return;
        if (auto dialog = inspectors.value(view)) { dialog->raise(); dialog->activateWindow(); return; }
        auto dialog = new QDialog(view);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("Developer tools — Synterra Surf" + QString(privateMode ? " · Private" : ""));
        dialog->resize(1000, 650);
        auto layout = new QVBoxLayout(dialog); layout->setContentsMargins(0, 0, 0, 0);
        auto tools = new QWebEngineView(dialog); tools->setPage(new QWebEnginePage(profile, tools)); layout->addWidget(tools);
        view->page()->setDevToolsPage(tools->page());
        inspectors.insert(view, dialog);
        QPointer<QWebEnginePage> inspected(view->page());
        connect(dialog, &QObject::destroyed, this, [this, view, inspected] {
            if (inspected) inspected->setDevToolsPage(nullptr);
            inspectors.remove(view);
        });
        dialog->show();
    }

    void fullscreen() {
        if (isFullScreen()) { if (normalState.testFlag(Qt::WindowMaximized)) showMaximized(); else showNormal(); }
        else { normalState = windowState(); showFullScreen(); }
    }
    void savePdf() {
        if (!current()) return;
        const QString path = smokePdfPath.isEmpty() ? QFileDialog::getSaveFileName(this,
            privateMode ? "Save PDF — this file will remain on disk" : "Save page as PDF",
            QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).filePath("page.pdf"), "PDF documents (*.pdf)") : smokePdfPath;
        if (path.isEmpty()) return;
        const auto destination = path.endsWith(".pdf", Qt::CaseInsensitive) ? path : path + ".pdf";
        if (destination != path && QFileInfo::exists(destination) && QMessageBox::question(this, "Replace PDF", "The PDF file already exists. Replace it?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        QPointer<Surf> owner(this);
        current()->page()->printToPdf([owner, destination](const QByteArray &bytes) {
            if (!owner) return;
            QSaveFile file(destination);
            const bool saved = !bytes.isEmpty() && file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
            owner->statusBar()->showMessage(saved ? "PDF saved" : "Could not save PDF", 5000);
            if (!owner->smokePdfPath.isEmpty()) {
                QFile check(destination); owner->smokePdfSaved = saved && check.open(QIODevice::ReadOnly) && check.read(5) == "%PDF-" && check.size() > 1024;
            } else if (!saved) QMessageBox::warning(owner, "PDF export", "The page could not be saved. Check the destination and try again after the page has loaded.");
        });
    }
    void manageBookmarks() {
        QDialog dialog(this); dialog.setWindowTitle("Manage bookmarks"); dialog.resize(620, 430);
        QVBoxLayout layout(&dialog); QLineEdit filter; filter.setPlaceholderText("Search bookmarks"); QListWidget list;
        layout.addWidget(&filter); layout.addWidget(&list);
        for (const auto &url : settings.value("bookmarks").toStringList()) list.addItem(url);
        QDialogButtonBox buttons; auto open = buttons.addButton("Open", QDialogButtonBox::ActionRole);
        auto remove = buttons.addButton("Remove selected", QDialogButtonBox::ActionRole);
        buttons.addButton(QDialogButtonBox::Close); layout.addWidget(&buttons);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(&filter, &QLineEdit::textChanged, &dialog, [&list](const QString &text) {
            for (int i = 0; i < list.count(); ++i) list.item(i)->setHidden(!list.item(i)->text().contains(text, Qt::CaseInsensitive));
        });
        auto visit = [this, &list, &dialog] { if (list.currentItem()) { const auto url = destination(list.currentItem()->text()); if (webAddress(url)) { addTab(url); dialog.accept(); } } };
        connect(open, &QPushButton::clicked, &dialog, visit);
        connect(&list, &QListWidget::itemActivated, &dialog, [visit](QListWidgetItem *) { visit(); });
        connect(remove, &QPushButton::clicked, &dialog, [this, &list] {
            if (!list.currentItem()) return;
            auto saved = settings.value("bookmarks").toStringList(); saved.removeAll(list.currentItem()->text());
            settings.setValue("bookmarks", saved); delete list.takeItem(list.currentRow());
        });
        dialog.exec();
    }

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
        QCheckBox notices("Show site and search notices"); notices.setChecked(noticesEnabled);
        QCheckBox searches("Include search-topic notices"); searches.setChecked(searchNotices);
        QCheckBox playful("Include playful messages"); playful.setChecked(playfulNotices);
        QLabel noticePrivacy("Notices are matched on this device using site addresses and supported search URLs.\nSurf does not scan page content or send notice checks to a server."); noticePrivacy.setWordWrap(true);
        QLabel note("Private windows never save browsing history or sessions.\nDownloads and bookmarks you save are kept on disk."); note.setWordWrap(true);
        QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        layout.addWidget(&restore); layout.addWidget(&dark); layout.addWidget(&notices); layout.addWidget(&searches); layout.addWidget(&playful); layout.addWidget(&noticePrivacy); layout.addWidget(&note); layout.addWidget(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        settings.setValue("restoreSession", restore.isChecked());
        if (!restore.isChecked()) settings.remove("session");
        settings.setValue("darkToolbar", dark.isChecked()); applyStyle(); saveSession();
        noticesEnabled = notices.isChecked(); searchNotices = searches.isChecked(); playfulNotices = playful.isChecked();
        settings.setValue("noticesEnabled", noticesEnabled); settings.setValue("searchNotices", searchNotices); settings.setValue("playfulNotices", playfulNotices);
        syncTopicNotice();
    }
    void applyStyle() {
        const bool dark = privateMode || settings.value("darkToolbar", false).toBool() || GlassAppearance::dark();
        const QString noticeStyle = dark
            ? "QWidget#surfTopicNotice {background:#1b3546; color:#ebf7ff; border-bottom:1px solid #53788d;} QWidget#surfTopicNotice[tone=\"warning\"] {background:#493a20; border-bottom:1px solid #aa8645;} QWidget#surfTopicNotice QLabel {background:transparent; color:#ebf7ff;} QWidget#surfTopicNotice QPushButton {background:rgba(255,255,255,18); color:#ebf7ff; border:1px solid #7791a0; border-radius:6px; padding:5px 9px;} QWidget#surfTopicNotice QPushButton:hover {background:rgba(255,255,255,40);}"
            : "QWidget#surfTopicNotice {background:#e5f3fb; color:#183b50; border-bottom:1px solid #a3c9de;} QWidget#surfTopicNotice[tone=\"warning\"] {background:#fff1cf; border-bottom:1px solid #d8b563;} QWidget#surfTopicNotice QLabel {background:transparent; color:#183b50;} QWidget#surfTopicNotice QPushButton {background:rgba(255,255,255,128); color:#183b50; border:1px solid #a2bdca; border-radius:6px; padding:5px 9px;} QWidget#surfTopicNotice QPushButton:hover {background:#ffffff;}";
        setStyleSheet((dark
            ? "QToolBar {background:#1d2940; color:#e6f0ff; padding:7px; spacing:6px;} QLineEdit {background:#273955; color:#f1f6ff; border:1px solid #5476a0; border-radius:10px; padding:8px 12px; min-width:180px;} QToolButton {color:#e6f0ff; padding:7px; border-radius:6px;} QToolButton:hover {background:#354b70;} QTabBar::tab {padding:9px 17px;}"
            : "QToolBar {background:#e7f0fa; padding:7px; spacing:6px; border-bottom:1px solid #bdcede;} QLineEdit {background:white; color:#19354e; border:1px solid #b6cee4; border-radius:10px; padding:8px 12px; min-width:180px;} QToolButton {padding:7px; border-radius:6px;} QToolButton:hover {background:#cfe4f6;} QTabBar::tab {padding:9px 17px;}") + noticeStyle);
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
    void syncNavigation(bool changedTab = false) {
        auto view = current();
        if (!view) return;
        if (changedTab || !address->hasFocus()) address->setText(view->url() == homeUrl ? QString() : withoutCredentials(view->url()).toDisplayString());
        backAction->setEnabled(view->history()->canGoBack());
        forwardAction->setEnabled(view->history()->canGoForward());
        const bool loading = view->page()->isLoading();
        reloadAction->setText(loading ? "Stop loading" : "Reload");
        reloadAction->setIcon(QIcon::fromTheme(loading ? "process-stop" : "view-refresh"));
        pageProgress->setValue(view->property("surfProgress").toInt()); pageProgress->setVisible(loading);
        muteAction->setChecked(view->page()->isAudioMuted());
        noticeText->setText(view->property("surfError").toString()); noticeBar->setVisible(!noticeText->text().isEmpty());
        syncTopicNotice();
        zoomLabel->setText(QString::number(qRound(view->zoomFactor() * 100)) + "%");
        setWindowTitle((view->title().isEmpty() ? "New tab" : view->title()) + " — Synterra Surf" + (privateMode ? " · Private" : ""));
        statusBar()->showMessage(view->url().scheme() == "https" ? "HTTPS" : view->url() == homeUrl ? privateMode ? "Private window · downloads are saved to disk" : "Welcome to Synterra Surf" : view->url().scheme().toUpper());
        if (loading) statusBar()->showMessage(QString("Loading %1% …").arg(pageProgress->value()));
        else if (view->property("surfStopped").toBool()) statusBar()->showMessage("Loading stopped. Reload to try again.");
    }
    void closeTab(int index) {
        QWidget *view = tabs->widget(index);
        if (!view) return;
        const auto url = qobject_cast<QWebEngineView *>(view)->url();
        if (webAddress(url)) { closedTabs.append(url.toString()); if (closedTabs.size() > 10) closedTabs.removeFirst(); }
        tabs->removeTab(index);
        topicStates.remove(qobject_cast<QWebEngineView *>(view)); topicVariants.remove(qobject_cast<QWebEngineView *>(view));
        if (auto inspector = inspectors.take(qobject_cast<QWebEngineView *>(view))) delete inspector;
        delete view;
        if (!tabs->count()) addTab(homeUrl);
    }
    void navigate() {
        QUrl url = destination(address->text());
        if (!url.isValid() || url.isEmpty()) {
            statusBar()->showMessage("Enter a web address or search terms.", 5000);
            return;
        }
        current()->setFocus(); current()->load(url);
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
        menu.addAction("Manage bookmarks", this, [this] { manageBookmarks(); });
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
        noticesEnabled = settings.value("noticesEnabled", true).toBool(); searchNotices = settings.value("searchNotices", true).toBool(); playfulNotices = settings.value("playfulNotices", true).toBool();
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
        pageProgress->setRange(0, 100); pageProgress->setTextVisible(false); pageProgress->setFixedHeight(3); pageProgress->hide();
        auto noticeLayout = new QHBoxLayout(noticeBar);
        noticeText->setTextFormat(Qt::PlainText); noticeText->setWordWrap(true);
        auto retry = new QPushButton("Reload", noticeBar), dismiss = new QPushButton("Dismiss", noticeBar);
        noticeLayout->addWidget(noticeText, 1); noticeLayout->addWidget(retry); noticeLayout->addWidget(dismiss); noticeBar->hide();
        connect(retry, &QPushButton::clicked, this, [this] { if (current()) current()->reload(); });
        connect(dismiss, &QPushButton::clicked, this, [this] { if (current()) current()->setProperty("surfError", QString()); noticeBar->hide(); });
        topicBar->setObjectName("surfTopicNotice");
        auto topicLayout = new QHBoxLayout(topicBar); topicLayout->setContentsMargins(14, 10, 14, 10); topicLayout->setSpacing(12);
        auto textLayout = new QVBoxLayout; textLayout->setSpacing(3);
        topicHeading->setTextFormat(Qt::PlainText); topicText->setTextFormat(Qt::PlainText);
        topicHeading->setWordWrap(true); topicText->setWordWrap(true);
        topicHeading->setTextInteractionFlags(Qt::TextSelectableByMouse); topicText->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto headingFont = topicHeading->font(); headingFont.setBold(true); topicHeading->setFont(headingFont);
        auto iconFont = topicIcon->font(); iconFont.setPointSize(22); topicIcon->setFont(iconFont);
        topicIcon->setMinimumWidth(28); textLayout->addWidget(topicHeading); textLayout->addWidget(topicText);
        topicLayout->addWidget(topicIcon); topicLayout->addLayout(textLayout, 1); topicLayout->addWidget(topicSource); topicLayout->addWidget(topicNext); topicLayout->addWidget(topicDismiss);
        topicDismiss->setAccessibleName("Dismiss this Surf notice"); topicDismiss->setToolTip("Hide this topic in this tab until you choose Show notices for this tab.");
        topicNext->setAccessibleName("Next matching Surf notice");
        connect(topicDismiss, &QPushButton::clicked, this, [this] { dismissTopicNotice(); });
        connect(topicNext, &QPushButton::clicked, this, [this] { if (current()) { ++topicStates[current()].selected; syncTopicNotice(); } });
        connect(topicSource, &QPushButton::clicked, this, [this] { const QUrl source = currentNoticeSource; if (!source.isEmpty()) addTab(source); });
        topicBar->hide();
        vertical->addWidget(pageProgress); vertical->addWidget(noticeBar); vertical->addWidget(topicBar); vertical->addWidget(tabs); vertical->addWidget(findBar); setCentralWidget(central);
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
        reloadAction = bar->addAction(QIcon::fromTheme("view-refresh"), "Reload", this, [this] { reloadOrStop(); });
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
        muteAction = menu->addAction("Mute this tab", this, [this](bool muted) { if (current()) current()->page()->setAudioMuted(muted); });
        muteAction->setCheckable(true);
        menu->addAction("Manage bookmarks", this, [this] { manageBookmarks(); });
        menu->addAction("Save page as PDF", this, [this] { savePdf(); });
        menu->addAction("Full screen (F11)", this, [this] { fullscreen(); });
        menu->addAction("Developer tools (F12)", this, [this] { developerTools(); });
        menu->addAction("Show notices for this tab", this, [this] { if (current()) { noticesEnabled = true; topicStates[current()].dismissed.clear(); syncTopicNotice(); } });
        auto noticesAction = menu->addAction("Site and search notices"); noticesAction->setCheckable(true);
        connect(menu, &QMenu::aboutToShow, this, [this, noticesAction] { noticesAction->setChecked(noticesEnabled); });
        connect(noticesAction, &QAction::triggered, this, [this](bool enabled) { noticesEnabled = enabled; if (!privateMode) settings.setValue("noticesEnabled", enabled); syncTopicNotice(); });
        menu->addSeparator();
        menu->addAction("Settings", this, [this] { preferences(); })->setEnabled(!privateMode);
        menu->addAction("About Surf", this, [this] {
            QMessageBox::about(this, "Synterra Surf", QString("<h2>Synterra Surf %1</h2><p>Synterra 2.0 · Build 2020 · Variscite</p><p>Powered by Qt WebEngine. Private windows use an isolated memory profile; downloaded files and saved bookmarks remain on disk.</p><p>Ctrl+L address · Ctrl+T new tab · Ctrl+D bookmark<br>Ctrl+F find · Ctrl+H history · Ctrl+J downloads<br>Ctrl+Shift+T reopen · Ctrl+Shift+N private window<br>Ctrl+Shift+M mute · F12 developer tools<br>Ctrl+W close · Ctrl+R reload · Ctrl++ / Ctrl+- zoom</p>").arg(QStringLiteral(SURF_VERSION)));
        });
        applyStyle(); GlassAppearance::watch(this, [this] { applyStyle(); });
        zoomLabel->setMinimumWidth(48); statusBar()->addPermanentWidget(zoomLabel);
        connect(address, &QLineEdit::returnPressed, this, [this] { navigate(); });
        connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int index) { closeTab(index); });
        connect(tabs, &QTabWidget::currentChanged, this, [this] { syncNavigation(true); findResult->clear(); if (findBar->isVisible()) find(); });
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
        shortcut("Ctrl+P", [this] { savePdf(); });
        shortcut("F11", [this] { fullscreen(); });
        shortcut("F12", [this] { developerTools(); });
        shortcut("Ctrl+Shift+I", [this] { developerTools(); });
        shortcut("Ctrl+Shift+M", [this] { muteAction->trigger(); });
        shortcut("Ctrl+Shift+R", [this] { current()->triggerPageAction(QWebEnginePage::ReloadAndBypassCache); });
        shortcut("Escape", [this] { if (isFullScreen()) fullscreen(); else if (findBar->isVisible()) { findBar->hide(); current()->findText({}); current()->setFocus(); } else if (current()->page()->isLoading()) reloadOrStop(); });
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
        for (auto dialog : inspectors.values()) if (dialog) delete dialog;
        while (tabs->count()) { auto view = tabs->widget(0); tabs->removeTab(0); delete view; }
        delete profile;
    }
    void reopen() { if (!closedTabs.isEmpty()) addTab(QUrl(closedTabs.takeLast())); }
    QStringList startupTabs() { return savedTabs(settings, privateMode); }
    void isolateSmokeRequests() { profile->setUrlRequestInterceptor(new FixtureRequests(profile)); }
    bool smokeWidgets() {
        activateWindow(); current()->setFocus(); QTest::qWait(100); QTest::keyClick(current(), Qt::Key_F, Qt::ControlModifier); findInput->setText("Synterra");
        if (!findBar->isVisible()) { std::cerr << "FAIL: Ctrl+F did not reveal find bar.\n"; return false; }
        const int count = tabs->count(); addTab(homeUrl); closeTab(tabs->currentIndex());
        if (tabs->count() != count) { std::cerr << "FAIL: tab closure.\n"; return false; }
        Surf privateWindow(true);
        if (!privateWindow.profile->isOffTheRecord() || privateWindow.profile->persistentCookiesPolicy() != QWebEngineProfile::NoPersistentCookies || !privateWindow.startupTabs().isEmpty()) { std::cerr << "FAIL: private profile isolation.\n"; return false; }
        QTest::keyClick(current(), Qt::Key_F11); QTest::qWait(100);
        if (!isFullScreen()) { std::cerr << "FAIL: F11 fullscreen.\n"; return false; }
        QTest::keyClick(current(), Qt::Key_Escape); QTest::qWait(100);
        if (isFullScreen()) { std::cerr << "FAIL: Escape fullscreen exit.\n"; return false; }
        current()->setZoomFactor(1.2); QTest::qWait(100);
        if (zoomLabel->text() != "120%") { std::cerr << "FAIL: zoom indicator.\n"; return false; }
        current()->setZoomFactor(1);
        const auto originalBookmarks = settings.value("bookmarks");
        settings.setValue("bookmarks", QStringList{"https://example.org/one", "https://example.org/two"});
        bool bookmarkCheck = false;
        QTimer::singleShot(100, this, [this, &bookmarkCheck] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto list = dialog->findChild<QListWidget *>(); auto filter = dialog->findChild<QLineEdit *>();
            if (!list || !filter) { dialog->reject(); return; }
            filter->setText("two");
            if (list->count() == 2 && list->item(0)->isHidden() && !list->item(1)->isHidden()) {
                list->setCurrentRow(1);
                for (auto button : dialog->findChildren<QPushButton *>()) if (button->text() == "Remove selected") button->click();
                bookmarkCheck = settings.value("bookmarks").toStringList() == QStringList{"https://example.org/one"};
            }
            dialog->reject();
        });
        manageBookmarks(); settings.setValue("bookmarks", originalBookmarks);
        if (!bookmarkCheck) { std::cerr << "FAIL: bookmark search/removal.\n"; return false; }
        return true;
    }
    QString findSummary() const { return findResult->text(); }
    void smokeDownload(const QUrl &url, const QString &path) { smokeDownloadPath = path; current()->page()->download(url); }
    bool downloadTestPassed() const { return smokeDownloadSaved && activeDownloads == 0; }
    void smokePdf(const QString &path) { smokePdfPath = path; QApplication::setActiveWindow(this); current()->setFocus(); QTest::qWait(150); QTest::keyClick(current(), Qt::Key_P, Qt::ControlModifier); }
    bool pdfTestPassed() const { return smokePdfSaved; }
    bool smokeNotices() {
        auto wait = [](auto condition) { for (int i = 0; i < 120; ++i) { if (condition()) return true; QTest::qWait(25); } return condition(); };
        auto fail = [](const char *message) { std::cerr << "FAIL: notice " << message << '\n'; return false; };
        const auto original = current();
        const bool oldEnabled = noticesEnabled, oldSearch = searchNotices, oldPlayful = playfulNotices;
        const auto oldDark = settings.value("darkToolbar");
        noticesEnabled = searchNotices = playfulNotices = true;
        const auto capture = [this](const QString &name) {
            const auto directory = qEnvironmentVariable("SYNTERRA_SURF_NOTICE_SCREENSHOTS");
            if (directory.isEmpty()) return true;
            QDir().mkpath(directory); QTest::qWait(80);
            return grab().save(QDir(directory).filePath(name + ".png"));
        };
        auto view = addTab(QUrl(), true, false);
        const QString fixture = "<!doctype html><meta charset='utf-8'><title>Surf notice preview</title><style>body{font:18px sans-serif;background:#edf4f7;color:#234;padding:70px}h1{font-weight:500}</style><h1>Site and search notices</h1><p>This local test page makes no external requests.</p><p>The notice above belongs to Surf, outside the web page.</p>";
        view->setHtml(fixture, QUrl("https://www.roblox.com/"));
        if (!wait([&] { return topicBar->isVisible() && !view->page()->isLoading(); }) || !topicHeading->text().contains("Warning · Roblox · This site") || !topicSource->isVisible()) return fail("site warning/source");
        const auto source = currentNoticeSource;
        const auto tabCount = tabs->count(); topicSource->click();
        if (!wait([&] { return tabs->count() == tabCount + 1 && current() != view && current()->page()->requestedUrl() == source; })) return fail("source opens in a new tab");
        closeTab(tabs->currentIndex()); tabs->setCurrentWidget(view);
        view->setProperty("surfError", "Recovery test"); syncNavigation();
        if (!noticeBar->isVisible() || !topicBar->isVisible()) return fail("recovery coexistence");
        view->setProperty("surfError", QString()); syncNavigation();
        settings.setValue("darkToolbar", false); applyStyle();
        if (!capture("notices-warning-light")) return fail("light capture");
        settings.setValue("darkToolbar", true); applyStyle();
        if (!capture("notices-warning-dark")) return fail("dark capture");
        settings.setValue("darkToolbar", false); applyStyle();
        topicDismiss->click(); syncNavigation();
        if (topicBar->isVisible()) return fail("dismissal did not survive navigation refresh");
        auto other = addTab(QUrl(), true, false); other->setHtml(fixture, QUrl("https://www.roblox.com/"));
        if (!wait([&] { return topicBar->isVisible() && !other->page()->isLoading(); })) return fail("dismissal leaked to another tab");
        closeTab(tabs->indexOf(other)); tabs->setCurrentWidget(view);
        if (topicBar->isVisible()) return fail("dismissed notice returned on tab switch");
        topicStates[view].dismissed.clear(); syncTopicNotice();
        if (!topicBar->isVisible()) return fail("restore notice");
        view->setHtml(fixture, QUrl("https://duckduckgo.com/?q=steam+ao3"));
        if (!wait([&] { return !view->page()->isLoading() && topicHeading->text().contains("Archive of Our Own · Search topic"); }) || !topicNext->isVisible()) return fail("search topic/priority");
        topicNext->click();
        if (!topicHeading->text().contains("Information · Steam") || topicSource->isVisible() || topicBar->property("tone") != "information") return fail("next/playful notice");
        if (!capture("notices-search")) return fail("search capture");
        resize(680, 790); QTest::qWait(80);
        if (topicDismiss->geometry().right() > topicBar->width() || topicText->width() < 120 || !capture("notices-narrow")) return fail("narrow layout");
        resize(1180, 790);
        playfulNotices = false; syncTopicNotice();
        if (!topicHeading->text().contains("Archive of Our Own") || topicNext->isVisible()) return fail("playful preference");
        searchNotices = false; syncTopicNotice();
        if (topicBar->isVisible()) return fail("search preference");
        searchNotices = playfulNotices = true;
        topicVariants[view].insert("ace-attorney", 0);
        view->setHtml(fixture, QUrl("https://duckduckgo.com/?q=ace+attorney"));
        if (!wait([&] { return !view->page()->isLoading() && topicText->text() == "when is ace attorney 7 coming out"; })) return fail("rare copy");
        syncNavigation(); if (topicText->text() != "when is ace attorney 7 coming out") return fail("rare copy flicker");
        noticesEnabled = false; syncTopicNotice(); if (topicBar->isVisible()) return fail("disable notices");
        noticesEnabled = true;
        view->setHtml(fixture, QUrl("https://roblox.com.evil.example/?q=ao3"));
        if (!wait([&] { return view->url().host() == "roblox.com.evil.example" && !view->page()->isLoading(); })) return fail("unrelated fixture");
        syncTopicNotice(); if (topicBar->isVisible()) return fail("spoofed site");
        {
            Surf privateWindow(true); privateWindow.noticesEnabled = true; privateWindow.isolateSmokeRequests();
            auto privateView = privateWindow.addTab(QUrl(), true, false);
            privateView->setHtml(fixture, QUrl("https://www.roblox.com/")); privateWindow.show();
            if (!wait([&] { return privateWindow.topicBar->isVisible() && !privateView->page()->isLoading(); })) return fail("private notice");
            privateWindow.topicDismiss->click();
            if (privateWindow.topicBar->isVisible() || !privateWindow.profile->isOffTheRecord()) return fail("private dismissal");
            privateWindow.close();
        }
        closeTab(tabs->indexOf(view)); tabs->setCurrentWidget(original);
        noticesEnabled = oldEnabled; searchNotices = oldSearch; playfulNotices = oldPlayful;
        if (oldDark.isValid()) settings.setValue("darkToolbar", oldDark); else settings.remove("darkToolbar");
        applyStyle(); syncNavigation();
        std::cout << "PASS: site/search notices, warning priority, sources, cycling, dismissal/restore, tab/private isolation, preferences, rare copy, dark/light and narrow layouts.\n";
        return true;
    }
    bool smokeWebsite(const QUrl &base) {
        auto wait = [](auto condition) {
            for (int i = 0; i < 200; ++i) { if (condition()) return true; QTest::qWait(25); }
            return condition();
        };
        auto evaluate = [&](QWebEngineView *view, const QString &script) {
            auto state = std::make_shared<std::pair<bool, QVariant>>();
            view->page()->runJavaScript(script, [state](const QVariant &result) { state->first = true; state->second = result; });
            wait([state] { return state->first; });
            return state->second;
        };
        auto click = [&](QWebEngineView *view, const QString &selector, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
            const auto point = evaluate(view, QString("(()=>{const r=document.querySelector('%1').getBoundingClientRect();return [r.x+r.width/2,r.y+r.height/2]})()").arg(selector)).toList();
            if (point.size() != 2) return false;
            QTest::mouseClick(view->focusProxy(), Qt::LeftButton, modifiers, QPoint(point[0].toInt(), point[1].toInt())); return true;
        };
        auto fail = [](const char *message) { std::cerr << "FAIL: " << message << '\n'; return false; };
        const int count = tabs->count() + 1;
        auto view = addTab(base.resolved(QUrl("/page")));
        if (!wait([&] { return view->title() == "Surf JavaScript check" && !view->page()->isLoading(); }) ||
            !wait([&] { return evaluate(view, "document.body.dataset.ready === 'yes'").toBool(); })) return fail("async JavaScript/fetch/DOM/localStorage");
        const bool popupAttempted = wait([&] { return evaluate(view, "!!document.body.dataset.popupBlocked").toBool(); });
        if (!popupAttempted || tabs->count() != count || !statusBar()->currentMessage().contains("popup blocked")) {
            std::cerr << "Popup diagnostic: attempted=" << popupAttempted << " tabs=" << tabs->count() << " expected=" << count << " status=" << statusBar()->currentMessage().toStdString() << '\n';
            return fail("automatic popup blocking");
        }
        if (!click(view, "#popup") || !wait([&] { return tabs->count() == count + 1 && current()->title() == "Surf popup check"; })) return fail("user-triggered JavaScript popup");
        if (!evaluate(current(), "!!window.opener").toBool() || !click(current(), "#close") || !wait([&] { return tabs->count() == count; })) return fail("popup opener/close lifecycle");
        tabs->setCurrentWidget(view);
        if (!click(view, "#background", Qt::ControlModifier) || !wait([&] { return tabs->count() == count + 1; }) || current() != view) return fail("background tab keeps foreground page");
        closeTab(tabs->count() - 1);
        activateWindow(); view->setFocus(); QTest::qWait(100);
        QTest::keyClick(view, Qt::Key_M, Qt::ControlModifier | Qt::ShiftModifier);
        if (!view->page()->isAudioMuted() || !muteAction->isChecked() || !tabs->tabToolTip(tabs->indexOf(view)).contains("Muted")) return fail("mute tab shortcut/state");
        auto other = addTab(homeUrl, false);
        if (other->page()->isAudioMuted()) return fail("mute leaked to another tab");
        closeTab(tabs->indexOf(other));
        QTest::keyClick(view, Qt::Key_M, Qt::ControlModifier | Qt::ShiftModifier);
        if (view->page()->isAudioMuted()) return fail("unmute tab");
        QTest::keyClick(view, Qt::Key_L, Qt::ControlModifier); address->setText("still typing.example"); view->setZoomFactor(1.1);
        if (address->text() != "still typing.example") return fail("navigation updates replaced an edited address");
        auto editedTab = addTab(homeUrl);
        if (!address->text().isEmpty()) return fail("new tab retained another tab's edited address");
        closeTab(tabs->indexOf(editedTab));
        view->setZoomFactor(1); view->setFocus();
        QTest::keyClick(view, Qt::Key_F12);
        if (!wait([&] { return view->page()->devToolsPage() && view->page()->devToolsPage()->url().scheme() == "devtools"; }) ||
            view->page()->devToolsPage()->inspectedPage() != view->page() || view->page()->devToolsPage()->profile() != profile) return fail("F12 inspector/profile attachment");
        inspectors.value(view)->close();
        if (!wait([&] { return !inspectors.value(view) && !view->page()->devToolsPage(); })) return fail("inspector detachment");
        activateWindow(); view->setFocus();
        view->load(base.resolved(QUrl("/slow")));
        if (!wait([&] { return view->page()->isLoading() && reloadAction->text() == "Stop loading" && pageProgress->isVisible(); })) return fail("loading controls");
        reloadAction->trigger();
        if (!wait([&] { return !view->page()->isLoading() && reloadAction->text() == "Reload" && !pageProgress->isVisible(); }) || noticeBar->isVisible()) return fail("stop treated as a load error");
        view->load(base.resolved(QUrl("/fail-once")));
        if (!wait([&] { return noticeBar->isVisible() && !view->page()->isLoading(); })) return fail("failed-load recovery banner");
        for (auto button : noticeBar->findChildren<QPushButton *>()) if (button->text() == "Reload") button->click();
        if (!wait([&] { return view->title() == "Surf JavaScript check" && !view->page()->isLoading() && !noticeBar->isVisible(); })) return fail("failed-load reload recovery");
        const qint64 process = view->page()->renderProcessPid();
        if (process <= 0 || QProcess::execute("kill", {"-KILL", QString::number(process)}) != 0 || !wait([&] { return noticeText->text().contains("web process stopped"); })) return fail("renderer crash notification");
        view->reload();
        if (!wait([&] { return !noticeBar->isVisible() && !view->page()->isLoading() && evaluate(view, "document.body.dataset.ready === 'yes'").toBool(); })) return fail("renderer reload recovery");
        developerTools(); QPointer<QDialog> inspector = inspectors.value(view);
        closeTab(tabs->indexOf(view));
        if (inspector || !inspectors.isEmpty()) return fail("closing inspected tab left developer tools open");
        tabs->setCurrentIndex(0); findBar->hide(); current()->findText({}); syncNavigation(); current()->setFocus(); QTest::qWait(100);
        return true;
    }
    QWebEngineView *addTab(const QUrl &url, bool select = true, bool load = true, bool scriptOpened = false) {
        auto view = new QWebEngineView;
        auto page = new QWebEnginePage(profile, view);
        view->setPage(page);
        page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
        page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        page->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
        connect(page, &QWebEnginePage::newWindowRequested, this, [this, view](QWebEngineNewWindowRequest &request) {
            if (!request.isUserInitiated()) {
                if (view == current()) statusBar()->showMessage("Automatic popup blocked. Use the website's link or button to open it.", 7000);
                return;
            }
            // Loading Home first races with script-created content and sign-in callbacks.
            const bool foreground = request.destination() != QWebEngineNewWindowRequest::InNewBackgroundTab;
            request.openIn(addTab(QUrl(), foreground, false, true)->page());
        });
        connect(page, &QWebEnginePage::windowCloseRequested, this, [this, view, scriptOpened] {
            if (scriptOpened) QTimer::singleShot(0, this, [this, guarded = QPointer<QWebEngineView>(view)] { if (guarded) closeTab(tabs->indexOf(guarded)); });
        });
        connect(page, &QWebEnginePage::audioMutedChanged, this, [this, view] { updateTab(view); if (view == current()) syncNavigation(); });
        connect(page, &QWebEnginePage::renderProcessTerminated, this, [this, view](QWebEnginePage::RenderProcessTerminationStatus, int) {
            view->setProperty("surfError", "This tab's web process stopped. Reload the page to recover."); if (view == current()) syncNavigation();
        });
        connect(page, &QWebEnginePage::permissionRequested, this, [this](QWebEnginePermission permission) {
            const auto answer = QMessageBox::question(this, "Website permission", permission.origin().toDisplayString() + " requests " + QVariant::fromValue(permission.permissionType()).toString() + ". Allow?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) permission.grant(); else permission.deny();
        });
        connect(page, &QWebEnginePage::findTextFinished, this, [this, view](const QWebEngineFindTextResult &result) {
            if (view == current()) findResult->setText(QString("%1 / %2").arg(result.activeMatch()).arg(result.numberOfMatches()));
        });
        connect(page, &QWebEnginePage::zoomFactorChanged, this, [this, view] { if (view == current()) syncNavigation(); });
        tabs->addTab(view, "New tab");
        if (select) tabs->setCurrentWidget(view);
        connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
            Q_UNUSED(title); updateTab(view);
            if (view == current()) syncNavigation();
        });
        connect(view, &QWebEngineView::iconChanged, this, [this, view] { updateTab(view); });
        connect(view, &QWebEngineView::urlChanged, this, [this, view] { if (view == current()) syncNavigation(); });
        connect(view, &QWebEngineView::loadStarted, this, [this, view] {
            view->setProperty("surfStopped", false); view->setProperty("surfError", QString()); view->setProperty("surfProgress", 0); if (view == current()) syncNavigation();
        });
        connect(view, &QWebEngineView::loadProgress, this, [this, view](int progress) { view->setProperty("surfProgress", progress); if (view == current()) syncNavigation(); });
        connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
            if (ok) recordVisit(settings, privateMode, view->url(), view->title());
            if (!ok && !view->property("surfStopped").toBool() && view->property("surfError").toString().isEmpty()) view->setProperty("surfError", "Page could not be loaded. Check the address and connection, then reload.");
            if (view == current()) syncNavigation();
        });
        if (load) view->load(url.isValid() ? url : homeUrl);
        return view;
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QString(argv[1]) == "--self-test") {
        QCoreApplication app(argc, argv);
        if (!SurfNotices::catalogError().isEmpty()) { std::cerr << SurfNotices::catalogError().toStdString() << '\n'; return 1; }
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
    GlassAppearance::syncPalette();
    app.setOrganizationName("Synterra");
    app.setApplicationName("SynterraSurf");
    app.setApplicationVersion(QStringLiteral(SURF_VERSION));
    app.setApplicationDisplayName("Synterra Surf");
    Surf window(app.arguments().contains("--private"));
    const bool smoke = app.arguments().contains("--smoke-test");
    if (smoke) window.isolateSmokeRequests();
    QTemporaryDir downloadDirectory;
    QTcpServer fixture;
    if (smoke) {
        if (!downloadDirectory.isValid() || !fixture.listen(QHostAddress::LocalHost)) return 1;
        QObject::connect(&fixture, &QTcpServer::newConnection, &app, [&fixture] {
            auto socket = fixture.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, request = QByteArray()]() mutable {
                request += socket->readAll(); if (!request.contains("\r\n\r\n")) return;
                const QByteArray path = request.split(' ').value(1); socket->disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);
                static int failures = 0;
                if (path == "/fail-once" && failures++ == 0) { socket->disconnectFromHost(); return; }
                QByteArray body, headers("Content-Type: text/html; charset=utf-8\r\n");
                if (path == "/download") { body = "Synterra Surf download fixture\n"; headers = "Content-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=fixture.txt\r\n"; }
                else if (path == "/data") { body = "{\"message\":\"JavaScript ready\"}"; headers = "Content-Type: application/json\r\n"; }
                else if (path == "/popup") body = "<!doctype html><title>Surf popup check</title><button id='close' onclick='window.close()'>Close this tab</button>";
                else { QFile page(":/surf/tests/web-fixture.html"); if (!page.open(QIODevice::ReadOnly)) return; body = page.readAll(); }
                auto respond = [socket, body, headers] { socket->write("HTTP/1.1 200 OK\r\n" + headers + "Content-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body); socket->disconnectFromHost(); };
                if (path == "/slow") QTimer::singleShot(3000, socket, respond); else respond();
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
        QTimer::singleShot(60000, &app, [&app] { std::cerr << "FAIL: smoke test timed out.\n"; app.exit(1); });
        QObject::connect(first, &QWebEngineView::loadFinished, &app, [first, &window, &app, &fixture, &downloadDirectory](bool ok) {
            if (!ok) { app.exit(1); return; }
            first->page()->runJavaScript("document.title === 'Synterra Surf' && !!document.querySelector('#search')", [&window, &app, &fixture, &downloadDirectory](const QVariant &result) {
                if (!result.toBool()) { app.exit(1); return; }
                if (!window.smokeWidgets()) { app.exit(1); return; }
                window.smokeDownload(QUrl(QString("http://127.0.0.1:%1/download").arg(fixture.serverPort())), downloadDirectory.filePath("fixture.txt"));
                window.smokePdf(downloadDirectory.filePath("fixture.pdf"));
                auto finished = new QTimer(&app); finished->setInterval(100);
                QObject::connect(finished, &QTimer::timeout, &app, [&window, &app, &fixture, finished, attempts = 0]() mutable {
                    if ((!window.downloadTestPassed() || !window.pdfTestPassed()) && ++attempts < 100) return;
                    finished->stop();
                    if (window.findSummary().isEmpty() || window.findSummary().endsWith(" / 0")) { std::cerr << "FAIL: find results: " << window.findSummary().toStdString() << "\n"; app.exit(1); return; }
                    if (!window.downloadTestPassed()) { std::cerr << "FAIL: local HTTP download/progress completion.\n"; app.exit(1); return; }
                    if (!window.pdfTestPassed()) { std::cerr << "FAIL: Ctrl+P PDF export.\n"; app.exit(1); return; }
                    if (!window.smokeWebsite(QUrl(QString("http://127.0.0.1:%1/").arg(fixture.serverPort())))) { app.exit(1); return; }
                    if (!window.smokeNotices()) { app.exit(1); return; }
                    const auto path = qEnvironmentVariable("SYNTERRA_SURF_SCREENSHOT");
                    if (!path.isEmpty() && !window.grab().save(path)) { app.exit(1); return; }
                    std::cout << "PASS: Chromium rendering, find, tabs, private profile, downloads, fullscreen, zoom, bookmarks, PDF, async JavaScript/fetch/storage, popup/background/close, mute, address editing, DevTools lifecycle, stop/error and renderer recovery.\n"; app.exit(0);
                });
                finished->start();
            });
        }, Qt::SingleShotConnection);
    }
    window.show();
    return app.exec();
}
