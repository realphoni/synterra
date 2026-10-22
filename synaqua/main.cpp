#include "shell.h"
#include "docksettings.h"
#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QFont>
#include <QQuickItem>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>
#include <QtTest/QTest>
#include <KGlobalAccel>
#include <KWindowSystem>
#include <LayerShellQt/Window>
#include <iostream>

int main(int argc, char **argv) {
    bool preview = false, test = false, layerTest = false;
    for (int i = 1; i < argc; ++i) { preview |= QByteArray(argv[i]) == "--preview"; test |= QByteArray(argv[i]) == "--self-test"; layerTest |= QByteArray(argv[i]) == "--self-test-wayland"; }
    test |= layerTest;
    const bool windowed = preview || (test && !layerTest);
    QApplication app(argc, argv);
    // The dock uses simple 2D items. A raster scene graph also works on
    // basic virtual GPUs without inheriting a renderer override into apps.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) QQuickWindow::setSceneGraphBackend("software");
    app.setOrganizationName("Synterra"); app.setApplicationName(test ? "SynaquaTest" : "Synaqua");
    app.setApplicationDisplayName("Synaqua"); app.setDesktopFileName("synaqua"); app.setQuitOnLastWindowClosed(false);
    app.setFont(QFont("Noto Sans", 10));
    if (app.arguments().contains("--settings")) return showDockSettings();
    Synaqua backend(windowed);
    if (!windowed) {
        auto bus = QDBusConnection::sessionBus();
        if (!bus.registerService("org.synterra.Synaqua")) {
            QDBusInterface running("org.synterra.Synaqua", "/Synaqua", "org.synterra.Synaqua"); running.call("toggleMenu"); return 0;
        }
        bus.registerObject("/Synaqua", &backend, QDBusConnection::ExportAllSlots);
    }
    QQuickView view; backend.setView(&view); view.setTitle("Synaqua · Synterra 2.0 Build 2010");
    view.setColor(Qt::transparent); view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.rootContext()->setContextProperty("synaqua", &backend);
    if (!windowed) {
        view.setFlags(Qt::FramelessWindowHint | Qt::Tool);
        auto layer = LayerShellQt::Window::get(&view);
        layer->setScope("synaqua-dock"); layer->setLayer(LayerShellQt::Window::LayerTop);
        layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) | LayerShellQt::Window::AnchorBottom | LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight);
        layer->setExclusiveZone(108); layer->setExclusiveEdge(LayerShellQt::Window::AnchorBottom);
        layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone); layer->setActivateOnShow(false);
        layer->setCloseOnDismissed(false); layer->setScreen(app.primaryScreen());
    }
    view.setSource(QUrl("qrc:/synaqua/qml/Main.qml"));
    if (view.status() == QQuickView::Error) return 1;
    if (windowed) view.resize(1440, 900);
    else view.setGeometry(app.primaryScreen()->geometry());
    auto updateScreen = [&] { if (!windowed && app.primaryScreen()) {
        LayerShellQt::Window::get(&view)->setScreen(app.primaryScreen()); view.setGeometry(app.primaryScreen()->geometry());
    } };
    QObject::connect(&app, &QGuiApplication::primaryScreenChanged, &view, updateScreen);
    QObject::connect(app.primaryScreen(), &QScreen::geometryChanged, &view, updateScreen);
    auto shortcut = new QAction("Open Synaqua Start", &app); shortcut->setObjectName("synaquaStart");
    if (!windowed) KGlobalAccel::self()->setDefaultShortcut(shortcut, {QKeySequence("Meta+Space")});
    if (!windowed) KGlobalAccel::self()->setShortcut(shortcut, {QKeySequence("Meta+Space")});
    QObject::connect(shortcut, &QAction::triggered, &backend, &Synaqua::toggleMenu);
    view.show();
    if (test) QTimer::singleShot(900, &app, [&] {
        auto root = view.rootObject();
        auto start = root ? root->findChild<QQuickItem *>("startButton") : nullptr;
        auto search = root ? root->findChild<QQuickItem *>("appSearch") : nullptr;
        bool ok = backend.selfTest() && root && start && search;
        const auto directory = qEnvironmentVariable("SYNAQUA_SCREENSHOTS");
        auto capture = [&](const QString &name) { if (directory.isEmpty()) return true; QDir().mkpath(directory); return view.grabWindow().save(directory + '/' + name); };
        auto click = [&](QQuickItem *item) { QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint()); QTest::qWait(150); };
        auto typeText = [&](const QByteArray &text) { for (char ch : text) QTest::keyClick(&view, ch); };
        if (ok) { backend.showMenu(); QTest::qWait(150); ok &= root->property("menuOpen").toBool(); backend.hideMenu(); QTest::qWait(150); ok &= !root->property("menuOpen").toBool(); }
        ok = ok && capture("synaqua-desktop.png");
        if (ok) { click(start); ok &= root->property("menuOpen").toBool(); }
        if (ok) { click(search); typeText("terminal"); QTest::qWait(150); ok &= backend.query() == "terminal" && !backend.applications().isEmpty(); }
        if (ok) { QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier); typeText("__synaqua_no_such_app__"); QTest::qWait(150); ok &= backend.applications().isEmpty(); }
        if (ok) { QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier); QTest::keyClick(&view, Qt::Key_Backspace); QTest::qWait(150); ok &= !backend.applications().isEmpty(); }
        ok = ok && capture("synaqua-start.png");
        QTest::keyClick(&view, Qt::Key_Escape); QTest::qWait(150); ok &= !root->property("menuOpen").toBool();
        if (ok) { click(start); QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(20,20)); QTest::qWait(150); ok &= !root->property("menuOpen").toBool(); }
        const auto fixture = qEnvironmentVariable("SYNAQUA_LAUNCH_FIXTURE");
        const auto marker = qEnvironmentVariable("SYNAQUA_LAUNCH_MARKER");
        if (ok && !fixture.isEmpty() && !marker.isEmpty()) {
            click(start); click(search); typeText(fixture.toUtf8()); QTest::qWait(150); QTest::keyClick(&view, Qt::Key_Return);
            QElapsedTimer deadline; deadline.start(); while (!QFile::exists(marker) && deadline.elapsed() < 5000) QTest::qWait(50);
            QFile launched(marker); ok &= launched.open(QIODevice::ReadOnly) && launched.readAll() == "argument with spaces\n";
        }
        QSettings config; const auto originalDock=DockPreferences::read(config);
        const auto originalPins=config.value("dock/pins").toStringList();
        auto fits = [&] {
            auto dock=root->findChild<QQuickItem *>("dock");
            auto left=root->findChild<QQuickItem *>("leftPins");
            auto right=root->findChild<QQuickItem *>("rightPins");
            auto utilities=root->findChild<QQuickItem *>("dockUtilities");
            if(!dock || !left || !right || !utilities) return false;
            const auto center=start->mapToScene(QPointF(start->width()/2,0)).x();
            if(layerTest && LayerShellQt::Window::get(&view)->exclusionZone()!=qCeil(view.height()-dock->y()+6)) return false;
            return qAbs(center-view.width()/2.)<1 && dock->x()>=23 && dock->x()+dock->width()<=view.width()-23
                && right->x()+right->width()<=utilities->x() && left->x()>=0
                && dock->y()+dock->height()<=view.height() && dock->height()>=49;
        };
        DockSettingsWidget editor; DockPreferences compact; compact.scale=75; compact.width=70; compact.margin=12;
        editor.setPreferences(compact); ok &= backend.dockScale()==originalDock.scale; // editing alone must not apply
        ok &= editor.save(); QTest::qWait(400); ok &= backend.dockScale()==75 && backend.dockWidth()==70 && fits();
        ok &= capture("synaqua-compact.png");
        DockPreferences wide; wide.width=100; wide.spacing=6; wide.margin=8;
        editor.setPreferences(wide); ok &= editor.save(); QTest::qWait(400);
        ok &= backend.dockWidth()==100 && backend.dockSpacing()==6 && fits();
        ok &= capture("synaqua-wide.png");
        editor.resize(650,650); editor.show(); QTest::qWait(150);
        if(!directory.isEmpty()) ok &= editor.grab().save(directory+"/synaqua-dock-settings.png");
        editor.hide();
        Synaqua reopened(true); ok &= reopened.dockWidth()==100 && reopened.dockMargin()==8;
        config.setValue("dock/scale",999); config.setValue("dock/width",-1); config.setValue("dock/spacing","broken"); config.sync();
        QTest::qWait(400); ok &= backend.dockScale()==125 && backend.dockWidth()==45 && backend.dockSpacing()==3;
        // Exercise the maximum pin count at every layout extreme on a small display.
        auto clearPins=[&] { const auto pins=backend.leftPins()+backend.rightPins(); for(const auto &pin:pins) backend.togglePin(pin.toMap().value("desktopId").toString()); };
        clearPins(); backend.setQuery(""); const auto apps=backend.applications();
        for(int i=0;i<qMin(8,apps.size());++i) backend.togglePin(apps[i].toMap().value("desktopId").toString());
        if(!layerTest) view.resize(1024,768);
        for(int scale:{60,100,125}) for(int width:{45,85,100}) {
            DockPreferences edge; edge.scale=scale; edge.width=width; edge.spacing=16; edge.margin=48;
            edge.write(config); QTest::qWait(120); ok &= fits();
        }
        wide.clock=false; wide.branding=false; wide.windows=false; wide.write(config); QTest::qWait(200);
        auto clock=root->findChild<QQuickItem *>("clockButton"); auto strip=root->findChild<QQuickItem *>("windowStrip");
        ok &= clock && !clock->isVisible() && strip && !strip->isVisible();
        clearPins(); for(const auto &id:originalPins) backend.togglePin(id);
        originalDock.write(config); QTest::qWait(200);
        if(!layerTest) view.resize(1024, 768);
        QTest::qWait(150); ok &= qAbs(start->mapToScene(QPointF(start->width()/2,0)).x()-view.width()/2.)<1;
        std::cout << (ok ? "PASS" : "FAIL") << ": native Synaqua launcher, persistent dock settings/live reload, compact/wide layout, eight pins at size/width extremes, visibility and dismissal.\n";
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}
