#include "shell.h"
#include <QCollator>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QPainterPath>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <KIO/ApplicationLauncherJob>
#include <KIO/OpenUrlJob>
#include <KJob>
#include <KSycoca>
#include <KWindowEffects>
#include <KWindowSystem>
#include <LayerShellQt/Window>
#include <algorithm>

namespace {
QRegion rounded(const QRectF &rect) {
    QPainterPath path; path.addRoundedRect(rect, 20, 20);
    return QRegion(path.toFillPolygon().toPolygon());
}
}

Synaqua::Synaqua(bool preview, QObject *parent) : QObject(parent), m_preview(preview) {
    reload();
    const QStringList defaults = {"org.kde.dolphin.desktop", "synterra-surf.desktop", "org.kde.konsole.desktop",
        "synterra-hub.desktop", "systemsettings.desktop", "org.kde.kate.desktop"};
    const auto saved = m_settings.value("dock/pins", defaults).toStringList();
    for (const auto &id : saved) if (find(id) && !m_pins.contains(id) && m_pins.size() < 8) m_pins << id;
    auto watcher = new QFileSystemWatcher(this);
    const auto config = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir().mkpath(config); watcher->addPath(config);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this] { emit appearanceChanged(); });
    if (QFile::exists(config + "/kdeglobals")) watcher->addPath(config + "/kdeglobals");
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this, watcher, config] {
        if (QFile::exists(config + "/kdeglobals") && !watcher->files().contains(config + "/kdeglobals")) watcher->addPath(config + "/kdeglobals");
        emit appearanceChanged();
    });
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, [this] { reload(); emit applicationsChanged(); emit pinsChanged(); });
}

void Synaqua::reload() {
    m_apps.clear();
    for (const auto &service : KService::allServices()) {
        if (!service->isApplication() || service->noDisplay() || !service->showInCurrentDesktop() || !service->showOnCurrentPlatform() || service->name().isEmpty()) continue;
        const auto tryExec = service->property<QString>("TryExec");
        if (!tryExec.isEmpty() && QStandardPaths::findExecutable(tryExec).isEmpty()) continue;
        const auto id = service->storageId();
        if (std::any_of(m_apps.cbegin(), m_apps.cend(), [&id](const App &app) { return app.id == id; })) continue;
        m_apps << App{id, service->name(), service->icon(), service->genericName(), service};
    }
    QCollator collator; collator.setCaseSensitivity(Qt::CaseInsensitive); collator.setNumericMode(true);
    std::sort(m_apps.begin(), m_apps.end(), [&collator](const App &a, const App &b) { return collator.compare(a.name, b.name) < 0; });
}
const Synaqua::App *Synaqua::find(const QString &id) const {
    for (const auto &app : m_apps) if (app.id == id) return &app;
    return nullptr;
}
QVariantMap Synaqua::appMap(const App &app) const {
    return {{"desktopId", app.id}, {"name", app.name}, {"icon", app.icon.isEmpty() ? "application-x-executable" : app.icon}, {"description", app.description}, {"pinned", pinned(app.id)}};
}
QVariantList Synaqua::applications() const {
    QVariantList result; const auto needle = m_query.trimmed();
    for (const auto &app : m_apps) {
        if (needle.isEmpty() || (app.name + ' ' + app.description + ' ' + app.id + ' ' + app.service->keywords().join(' ')).contains(needle, Qt::CaseInsensitive)) result << appMap(app);
    }
    return result;
}
QVariantList Synaqua::pinSlice(int from, int to) const {
    QVariantList result;
    for (int i = from; i < to; ++i) if (const auto app = find(m_pins.at(i))) result << appMap(*app);
    return result;
}
QVariantList Synaqua::leftPins() const { return pinSlice(0, (m_pins.size() + 1) / 2); }
QVariantList Synaqua::rightPins() const { return pinSlice((m_pins.size() + 1) / 2, m_pins.size()); }
void Synaqua::setQuery(const QString &query) { if (query != m_query) { m_query = query; emit applicationsChanged(); } }
bool Synaqua::pinned(const QString &id) const { return m_pins.contains(id); }
void Synaqua::persistPins() { m_settings.setValue("dock/pins", m_pins); m_settings.sync(); emit pinsChanged(); emit applicationsChanged(); }
void Synaqua::togglePin(const QString &id) {
    if (!find(id)) { fail("This application is no longer installed."); return; }
    if (m_pins.contains(id)) m_pins.removeAll(id);
    else if (m_pins.size() < 8) m_pins << id;
    else { fail("The dock holds eight pinned apps. Unpin one before adding another."); return; }
    persistPins();
}
void Synaqua::fail(const QString &message) { m_error = message; emit errorChanged(); }
void Synaqua::clearError() { if (!m_error.isEmpty()) { m_error.clear(); emit errorChanged(); } }
bool Synaqua::dark() const {
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals");
    return file.open(QIODevice::ReadOnly) && file.readAll().contains("ColorScheme=SynterraGlassDark");
}
bool Synaqua::live() const { return QDir("/run/archiso").exists(); }
QString Synaqua::userName() const { return qEnvironmentVariable("USER", "Synterra"); }
void Synaqua::launch(const QString &id) {
    const auto app = find(id);
    if (!app) { fail("This application could not be found. Check that it is installed."); return; }
    auto job = new KIO::ApplicationLauncherJob(app->service, this);
    connect(job, &KJob::result, this, [this](KJob *result) { if (result->error()) fail(result->errorText()); });
    job->start();
}
void Synaqua::openHome() {
    auto job = new KIO::OpenUrlJob(QUrl::fromLocalFile(QDir::homePath()), "inode/directory", this);
    connect(job, &KJob::result, this, [this](KJob *result) { if (result->error()) fail(result->errorText()); }); job->start();
}
void Synaqua::openSessionDialog() {
    QDBusInterface prompt("org.kde.LogoutPrompt", "/LogoutPrompt", "org.kde.LogoutPrompt");
    auto watcher = new QDBusPendingCallWatcher(prompt.asyncCall("promptAll"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] { QDBusPendingReply<> reply = *watcher; if (reply.isError()) fail("Could not open session controls: " + reply.error().message()); watcher->deleteLater(); });
}
void Synaqua::lockScreen() {
    QDBusInterface saver("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver");
    auto watcher = new QDBusPendingCallWatcher(saver.asyncCall("Lock"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] { QDBusPendingReply<> reply = *watcher; if (reply.isError()) fail("Screen locking is unavailable in this session."); watcher->deleteLater(); });
}
void Synaqua::showDesktop() {
    QDBusInterface kwin("org.kde.KWin", "/KWin", "org.kde.KWin"); kwin.asyncCall("showDesktop", !kwin.property("showingDesktop").toBool());
}
void Synaqua::setGlass(bool darkMode) {
    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) { fail("Could not start the Glass appearance helper."); });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this, process](int code, QProcess::ExitStatus status) { if (code || status != QProcess::NormalExit) fail(QString::fromUtf8(process->readAllStandardError())); emit appearanceChanged(); process->deleteLater(); });
    process->start("synterra-appearance", {darkMode ? "dark" : "light"});
}
void Synaqua::openSettings(const QString &module) {
    const QStringList allowed = {"kcm_networkmanagement", "kcm_clock"};
    if (!allowed.contains(module)) { fail("Unknown settings page."); return; }
    if (!QProcess::startDetached("systemsettings", {module})) fail("System Settings could not be started.");
}
void Synaqua::updateRegions(const QRectF &dock, const QRectF &menu, const QRectF &toast, bool open) {
    if (!m_view || m_preview) return;
    const auto errorRegion = m_error.isEmpty() ? QRegion() : rounded(toast);
    const auto glass = rounded(dock) | (open ? rounded(menu) : QRegion()) | errorRegion;
    // Only the dock receives clicks when closed. An open Start menu catches
    // outside clicks so dismissal never falls through to another application.
    m_view->setMask(open ? QRegion(0, 0, m_view->width(), m_view->height()) : rounded(dock) | errorRegion);
    KWindowEffects::enableBlurBehind(m_view, true, glass);
    if (KWindowSystem::isPlatformWayland()) {
        auto layer = LayerShellQt::Window::get(m_view);
        layer->setKeyboardInteractivity(open ? LayerShellQt::Window::KeyboardInteractivityExclusive : LayerShellQt::Window::KeyboardInteractivityNone);
    }
    if (open) m_view->requestActivate();
}
void Synaqua::setMenuOpen(bool open) { if (m_menuOpen != open) { m_menuOpen = open; emit menuOpenChanged(); } }
void Synaqua::toggleMenu() { setMenuOpen(!m_menuOpen); }

bool Synaqua::selfTest() {
    if (m_apps.isEmpty()) return false;
    const auto before = m_pins; const auto id = m_apps.first().id;
    m_pins.clear(); togglePin(id); togglePin(id); if (!m_pins.isEmpty()) return false;
    togglePin(id); togglePin("../../bin/sh"); if (m_pins != QStringList{id} || m_error.isEmpty()) return false;
    clearError();
    if (m_apps.size() >= 9) {
        m_pins.clear();
        for (int i = 0; i < 8; ++i) togglePin(m_apps.at(i).id);
        togglePin(m_apps.at(8).id);
        if (m_pins.size() != 8 || m_error.isEmpty()) return false;
        clearError();
        QSettings persisted;
        if (persisted.value("dock/pins").toStringList() != m_pins) return false;
    }
    setQuery(m_apps.first().name.toUpper()); if (applications().isEmpty()) return false;
    setQuery("__synaqua_no_such_app__"); if (!applications().isEmpty()) return false;
    setQuery(""); m_pins = before; persistPins();
    return true;
}
