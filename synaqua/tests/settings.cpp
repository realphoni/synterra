#include "../dockconfig.h"
#include <QApplication>
#include <QCheckBox>
#include <QSpinBox>
#include <QtTest/QTest>
#include <KCModule>
#include <KPluginFactory>
#include <iostream>

int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("Synterra"); app.setApplicationName("SynaquaTest");
    if(argc!=2) return 2;
    QSettings settings; const auto original=DockPreferences::read(settings); const auto pins=settings.value("dock/pins");
    auto result=KPluginFactory::instantiatePlugin<KCModule>(KPluginMetaData(QString::fromLocal8Bit(argv[1])));
    if(!result) { std::cerr << result.errorString.toStdString() << '\n'; return 1; }
    auto module=result.plugin;
    module->load();
    auto size=module->widget()->findChild<QSpinBox *>("dockScale"); auto width=module->widget()->findChild<QSpinBox *>("dockWidth");
    bool ok=size && width && !module->needsSave();
    if(ok) {
        size->setValue(75); width->setValue(100); ok &= module->needsSave();
        module->save(); settings.sync(); auto saved=DockPreferences::read(settings);
        ok &= !module->needsSave() && saved.scale==75 && saved.width==100 && settings.value("dock/pins")==pins;
        size->setValue(120); module->load(); ok &= size->value()==75 && !module->needsSave();
        module->defaults(); ok &= module->needsSave() && size->value()==100 && width->value()==85;
        module->save(); settings.sync(); ok &= DockPreferences::read(settings)==DockPreferences{};
        module->widget()->resize(650,650); module->widget()->show(); QTest::qWait(250);
        const auto shot=qEnvironmentVariable("SYNAQUA_SETTINGS_SCREENSHOT");
        if(!shot.isEmpty()) ok &= module->widget()->grab().save(shot);
    }
    original.write(settings); delete module;
    std::cout << (ok ? "PASS" : "FAIL") << ": real KDE Settings plugin load, Apply, persistence, Reset, Defaults and unchanged pins.\n";
    return ok ? 0 : 1;
}
