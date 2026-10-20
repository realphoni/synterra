#pragma once
#include <QObject>
#include <QVariantList>
#include <QSettings>
#include <QQuickView>
#include <KService>
#include "dockconfig.h"

class Synaqua final : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.synterra.Synaqua")
    Q_PROPERTY(QVariantList applications READ applications NOTIFY applicationsChanged)
    Q_PROPERTY(QVariantList leftPins READ leftPins NOTIFY pinsChanged)
    Q_PROPERTY(QVariantList rightPins READ rightPins NOTIFY pinsChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY applicationsChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY appearanceChanged)
    Q_PROPERTY(bool live READ live CONSTANT)
    Q_PROPERTY(bool preview READ preview CONSTANT)
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool menuOpen READ isMenuOpen WRITE setMenuOpen NOTIFY menuOpenChanged)
    Q_PROPERTY(int dockScale READ dockScale NOTIFY dockSettingsChanged)
    Q_PROPERTY(int dockWidth READ dockWidth NOTIFY dockSettingsChanged)
    Q_PROPERTY(int dockSpacing READ dockSpacing NOTIFY dockSettingsChanged)
    Q_PROPERTY(int dockMargin READ dockMargin NOTIFY dockSettingsChanged)
    Q_PROPERTY(bool showClock READ showClock NOTIFY dockSettingsChanged)
    Q_PROPERTY(bool showBranding READ showBranding NOTIFY dockSettingsChanged)
    Q_PROPERTY(bool showWindows READ showWindows NOTIFY dockSettingsChanged)
public:
    explicit Synaqua(bool preview, QObject *parent = nullptr);
    QVariantList applications() const;
    QVariantList leftPins() const;
    QVariantList rightPins() const;
    QString query() const { return m_query; }
    void setQuery(const QString &query);
    bool dark() const;
    bool live() const;
    bool preview() const { return m_preview; }
    QString userName() const;
    QString error() const { return m_error; }
    void setView(QQuickView *view) { m_view = view; }
    Q_INVOKABLE void launch(const QString &id);
    Q_INVOKABLE void togglePin(const QString &id);
    Q_INVOKABLE bool pinned(const QString &id) const;
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void openHome();
    Q_INVOKABLE void openSessionDialog();
    Q_INVOKABLE void lockScreen();
    Q_INVOKABLE void showDesktop();
    Q_INVOKABLE void setGlass(bool dark);
    Q_INVOKABLE void openSettings(const QString &module);
    Q_INVOKABLE void updateRegions(const QRectF &dock, const QRectF &menu, const QRectF &toast, bool open);
    bool selfTest();
    void setMenuOpen(bool open);
    int dockScale() const { return m_dock.scale; }
    int dockWidth() const { return m_dock.width; }
    int dockSpacing() const { return m_dock.spacing; }
    int dockMargin() const { return m_dock.margin; }
    bool showClock() const { return m_dock.clock; }
    bool showBranding() const { return m_dock.branding; }
    bool showWindows() const { return m_dock.windows; }
    Q_INVOKABLE void openDockSettings();
public slots:
    void reloadDockSettings();
    void toggleMenu();
    void showMenu() { setMenuOpen(true); }
    void hideMenu() { setMenuOpen(false); }
    bool isMenuOpen() const { return m_menuOpen; }
signals:
    void applicationsChanged();
    void pinsChanged();
    void appearanceChanged();
    void errorChanged();
    void menuOpenChanged();
    void dockSettingsChanged();
private:
    struct App { QString id, name, icon, description; KService::Ptr service; };
    QList<App> m_apps;
    QStringList m_pins;
    QSettings m_settings;
    DockPreferences m_dock;
    QString m_query, m_error;
    bool m_preview;
    bool m_menuOpen = false;
    QQuickView *m_view = nullptr;
    QVariantMap appMap(const App &app) const;
    const App *find(const QString &id) const;
    QVariantList pinSlice(int from, int to) const;
    void reload();
    void fail(const QString &message);
    void persistPins();
};
