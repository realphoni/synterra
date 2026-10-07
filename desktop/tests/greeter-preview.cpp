#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlPropertyMap>
#include <QAbstractListModel>
#include <QQuickWindow>
#include <QQuickItem>
#include <QImage>
#include <QTimer>
#include <QtTest/QTest>
#include <iostream>

class Sessions final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int lastIndex READ lastIndex CONSTANT)
public:
    int lastIndex() const { return 0; }
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 1; }
    QVariant data(const QModelIndex &index, int role) const override { return index.isValid() && role == Qt::UserRole ? QVariant("Plasma (Wayland)") : QVariant(); }
    QHash<int, QByteArray> roleNames() const override { return {{Qt::UserRole, "name"}}; }
};
class Greeter final : public QObject {
    Q_OBJECT
public:
    bool requested = false;
    Q_INVOKABLE void login(const QString &user, const QString &password, int session) { requested = user == "fixture" && password == "fixture-only" && session == 0; emit loginFailed(); }
signals:
    void loginFailed();
    void loginSucceeded();
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (app.arguments().size() != 4) return 2;
    QQmlPropertyMap user, config; user.insert("lastUser", "fixture"); config.insert("background", app.arguments().at(2)); config.insert("logo", app.arguments().at(3)); Sessions sessions; Greeter greeter;
    QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("userModel", &user); engine.rootContext()->setContextProperty("config", &config); engine.rootContext()->setContextProperty("sessionModel", &sessions); engine.rootContext()->setContextProperty("sddm", &greeter);
    engine.load(QUrl::fromLocalFile(app.arguments().at(1)));
    if (engine.rootObjects().isEmpty()) return 1;
    auto root = engine.rootObjects().first(); auto window = new QQuickWindow; root->setParent(window);
    auto item = qobject_cast<QQuickItem *>(root); if (!item) return 1;
    item->setParentItem(window->contentItem()); window->resize(1280, 800); window->show();
    QTimer::singleShot(500, &app, [&] {
        auto password = root->findChild<QObject *>("password"); auto login = root->findChild<QObject *>("loginButton");
        if (!password || !login || password->property("echoMode").toInt() != 2) { app.exit(1); return; }
        password->setProperty("text", "fixture-only"); QMetaObject::invokeMethod(login, "clicked");
        if (!greeter.requested || !password->property("text").toString().isEmpty()) { app.exit(1); return; }
        auto error = root->findChild<QObject *>("errorMessage"); if (!error || error->property("text").toString().isEmpty()) { app.exit(1); return; }
        error->setProperty("text", "");
        if (!window->grabWindow().save(qEnvironmentVariable("SYNTERRA_GREETER_SCREENSHOT"))) { app.exit(1); return; }
        std::cout << "PASS: actual login QML renders, session roles resolve, password is masked and failed authentication clears its field.\n"; app.quit();
    });
    QTimer::singleShot(5000, &app, [&] { app.exit(1); });
    const int status = app.exec(); delete window; return status;
}
#include "greeter-preview.moc"
