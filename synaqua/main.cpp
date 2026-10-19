#include "shell.h"
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
    bool preview = false, test = false;
    for (int i = 1; i < argc; ++i) { preview |= QByteArray(argv[i]) == "--preview"; test |= QByteArray(argv[i]) == "--self-test"; }
    QApplication app(argc, argv);
    // The preview uses simple 2D items. A raster scene graph also works on
    // basic virtual GPUs without inheriting a renderer override into apps.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) QQuickWindow::setSceneGraphBackend("software");
    app.setOrganizationName("Synterra"); app.setApplicationName(test ? "SynaquaTest" : "Synaqua");
    app.setApplicationDisplayName("Synaqua"); app.setDesktopFileName("synaqua"); app.setQuitOnLastWindowClosed(false);
    app.setFont(QFont("Noto Sans", 10));
    Synaqua backend(preview || test);
    if (!preview && !test) {
        auto bus = QDBusConnection::sessionBus();
        if (!bus.registerService("org.synterra.Synaqua")) {
            QDBusInterface running("org.synterra.Synaqua", "/Synaqua", "org.synterra.Synaqua"); running.call("toggleMenu"); return 0;
        }
        bus.registerObject("/Synaqua", &backend, QDBusConnection::ExportAllSlots);
    }
    QQuickView view; backend.setView(&view); view.setTitle("Synaqua · Synterra 1.1 Build 1165");
    view.setColor(Qt::transparent); view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.rootContext()->setContextProperty("synaqua", &backend);
    if (!preview && !test) {
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
    if (preview || test) view.resize(1440, 900);
    else view.setGeometry(app.primaryScreen()->geometry());
    auto updateScreen = [&] { if (!preview && !test && app.primaryScreen()) {
        LayerShellQt::Window::get(&view)->setScreen(app.primaryScreen()); view.setGeometry(app.primaryScreen()->geometry());
    } };
    QObject::connect(&app, &QGuiApplication::primaryScreenChanged, &view, updateScreen);
    QObject::connect(app.primaryScreen(), &QScreen::geometryChanged, &view, updateScreen);
    auto shortcut = new QAction("Open Synaqua Start", &app); shortcut->setObjectName("synaquaStart");
    if (!preview && !test) KGlobalAccel::self()->setDefaultShortcut(shortcut, {QKeySequence("Meta+Space")});
    if (!preview && !test) KGlobalAccel::self()->setShortcut(shortcut, {QKeySequence("Meta+Space")});
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
        view.resize(1024, 768); QTest::qWait(150); ok &= start->mapToScene(QPointF(start->width()/2,0)).x() > 500 && start->mapToScene(QPointF(start->width()/2,0)).x() < 524;
        std::cout << (ok ? "PASS" : "FAIL") << ": native Synaqua rendering, app search/empty state, bounded persistent pins, centered responsive Start, Escape and outside-click dismissal.\n";
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}
