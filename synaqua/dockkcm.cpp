#include "docksettings.h"
#include <KCModule>
#include <KPluginFactory>
#include <QMessageBox>
#include <QVBoxLayout>

class SynaquaDockKcm final : public KCModule {
    Q_OBJECT
public:
    SynaquaDockKcm(QObject *parent,const KPluginMetaData &data) : KCModule(qobject_cast<QWidget *>(parent),data) {
        setButtons(Apply | Default);
        auto layout=new QVBoxLayout(widget()); layout->setContentsMargins(0,0,0,0);
        m_editor=new DockSettingsWidget(widget()); layout->addWidget(m_editor);
        connect(m_editor,&DockSettingsWidget::edited,this,[this] { setNeedsSave(true); });
    }
    void load() override { m_editor->load(); setNeedsSave(false); }
    void save() override {
        if(m_editor->save()) setNeedsSave(false);
        else QMessageBox::warning(widget(),"Synaqua Dock","The dock settings could not be saved. Check that your configuration folder is writable.");
    }
    void defaults() override { m_editor->setPreferences(DockPreferences{}); setNeedsSave(true); }
private:
    DockSettingsWidget *m_editor;
};
K_PLUGIN_CLASS_WITH_JSON(SynaquaDockKcm,"dockkcm.json")
#include "dockkcm.moc"
