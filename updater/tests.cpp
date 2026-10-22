#include "release.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <archive.h>
#include <archive_entry.h>

class UpdaterTests : public QObject {
    Q_OBJECT
    Updates::System system{Updates::Version::parse("2.0", 2010), false};
    QJsonObject manifest() {
        return {{"schema",1},{"version","2.0"},{"build",2011},{"tag","v2.0.2011"},{"architecture","x86_64"},{"minimum_version","1.1"},
            {"package",QJsonObject{{"name","synterra-release-2.0.2011-1-x86_64.pkg.tar.zst"},{"size",100},{"sha256",QString(64,'a')}}}};
    }
    QJsonObject release() {
        const QString name="synterra-release-2.0.2011-1-x86_64.pkg.tar.zst";
        return {{"tag_name","v2.0.2011"},{"name","Synterra 2.0"},{"draft",false},{"prerelease",false},
            {"assets",QJsonArray{QJsonObject{{"name",name},{"state","uploaded"},{"size",100},{"digest","sha256:"+QString(64,'a')},
                {"browser_download_url","https://github.com/realphoni/synterra/releases/download/v2.0.2011/"+name}}}}};
    }
    void package(const QString &path, const QString &file, bool link=false, const QString &identity="synterra-release") {
        auto ar=archive_write_new(); archive_write_add_filter_zstd(ar); archive_write_set_format_pax_restricted(ar);
        QVERIFY(archive_write_open_filename(ar,path.toUtf8().constData()) == ARCHIVE_OK);
        const QByteArray info=("pkgname = "+identity+"\npkgver = 2.0.2011-1\narch = x86_64\n").toUtf8();
        for (const auto &item : QList<QPair<QString,QByteArray>>{{".PKGINFO",info},{file,"fixture"}}) {
            auto entry=archive_entry_new(); archive_entry_set_pathname(entry,item.first.toUtf8().constData());
            archive_entry_set_filetype(entry,AE_IFREG); archive_entry_set_perm(entry,0644);
            if(link && item.first==file) { archive_entry_set_filetype(entry,AE_IFLNK); archive_entry_set_symlink(entry,"/etc/shadow"); } else archive_entry_set_size(entry,item.second.size());
            QVERIFY(archive_write_header(ar,entry) == ARCHIVE_OK);
            if(!link || item.first!=file) archive_write_data(ar,item.second.constData(),item.second.size());
            archive_entry_free(entry);
        }
        archive_write_close(ar); archive_write_free(ar);
    }
private slots:
    void versions() {
        QVERIFY(Updates::Version::parse("1.1",1170)<system.version);
        QVERIFY(system.version<Updates::Version::parse("2.0",2011));
        QVERIFY(Updates::Version::parse("2.0",99999)<Updates::Version::parse("2.1",2100));
        QVERIFY(!(system.version<system.version));
        QVERIFY_EXCEPTION_THROWN(Updates::Version::parse("2.0;sh",2010),std::exception);
        QVERIFY_EXCEPTION_THROWN(Updates::Version::parse("2.0",0),std::exception);
    }
    void trustedManifest() {
        auto plan=Updates::parsePlan(release(),QJsonDocument(manifest()).toJson(),system); QCOMPARE(plan.version.build,2011);
        auto data=release(); data["prerelease"]=true;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(data,QJsonDocument(manifest()).toJson(),system),std::exception);
        data=release(); data["draft"]=true;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(data,QJsonDocument(manifest()).toJson(),system),std::exception);
        auto m=manifest(); m["build"]=2012;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(release(),QJsonDocument(m).toJson(),system),std::exception);
        m=manifest(); m["architecture"]="aarch64";
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(release(),QJsonDocument(m).toJson(),system),std::exception);
        m=manifest(); auto p=m["package"].toObject();p["sha256"]=QString(64,'b');m["package"]=p;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(release(),QJsonDocument(m).toJson(),system),std::exception);
        QVERIFY(Updates::parsePlan(release(),QJsonDocument(manifest()).toJson(),{Updates::Version::parse("1.1",1170),false}).version.build == 2011);
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(release(),QJsonDocument(manifest()).toJson(),{Updates::Version::parse("1.0",1000),false}),std::exception);
        QVERIFY_EXCEPTION_THROWN(Updates::parseObject("<html>error</html>"),std::exception);
    }
    void addresses() {
        QVERIFY(Updates::allowedUrl(QUrl("https://release-assets.githubusercontent.com/path")));
        for(const auto &url:{"http://github.com/a","https://evil.test/a","https://github.com.evil.test/a","https://user:pass@github.com/a","https://github.com:444/a","file:///etc/passwd"}) QVERIFY(!Updates::allowedUrl(QUrl(url)));
        auto data=release(); auto entries=data["assets"].toArray();auto a=entries[0].toObject();a["browser_download_url"]="https://github.com/another/repository/file";entries[0]=a;data["assets"]=entries;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(data,QJsonDocument(manifest()).toJson(),system),std::exception);
        data=release();entries=data["assets"].toArray();entries.append(entries[0]);data["assets"]=entries;
        QVERIFY_EXCEPTION_THROWN(Updates::parsePlan(data,QJsonDocument(manifest()).toJson(),system),std::exception);
    }
    void packages() {
        QTemporaryDir temp; auto path=temp.filePath("test.pkg.tar.zst");Updates::Plan plan;plan.version=Updates::Version::parse("2.0",2011);
        package(path,"usr/local/bin/synterra-updates");QCOMPARE(Updates::inspectPackage(path,plan).size(),1);
        for(const auto &name:{"etc/shadow","usr/local/bin/../../etc/shadow","/usr/local/bin/synterra-updates",".INSTALL","usr/share/synterra/../../etc/passwd"}) {
            package(path,name); QVERIFY_EXCEPTION_THROWN(Updates::inspectPackage(path,plan),std::exception);
        }
        package(path,"usr/local/bin/synterra-updates",true);QVERIFY_EXCEPTION_THROWN(Updates::inspectPackage(path,plan),std::exception);
        package(path,"usr/local/bin/synterra-updates",false,"another-package");QVERIFY_EXCEPTION_THROWN(Updates::inspectPackage(path,plan),std::exception);
        package(path,"usr/local/bin/synterra-updates");plan.version.build=2012;QVERIFY_EXCEPTION_THROWN(Updates::inspectPackage(path,plan),std::exception);
    }
    void networkAndCache() {
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray content="verified release fixture";int requests=0;
        connect(&server,&QTcpServer::newConnection,this,[&]{auto socket=server.nextPendingConnection();connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{
            const auto request=socket->readAll();if(!request.contains("\r\n\r\n"))return;++requests;
            if(request.startsWith("GET /slow")) return;
            QByteArray response;
            if(request.startsWith("GET /redirect"))response="HTTP/1.1 302 Found\r\nLocation: http://evil.test/file\r\nContent-Length: 0\r\n\r\n";
            else response="HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(content.size())+"\r\nConnection: close\r\n\r\n"+content;
            socket->write(response);socket->disconnectFromHost();
        });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);});
        const auto url=QUrl("http://127.0.0.1:"+QString::number(server.serverPort())+"/data");std::atomic_bool cancel=false;
        QCOMPARE(Updates::fetch(url,1024,cancel,true),content);
        QVERIFY_EXCEPTION_THROWN(Updates::fetch(url,3,cancel,true),std::exception);
        auto redir=url;redir.setPath("/redirect");QVERIFY_EXCEPTION_THROWN(Updates::fetch(redir,1024,cancel,true),std::exception);
        QTemporaryDir cache;Updates::Asset a{"fixture.pkg.tar.zst",QString::fromLatin1(QCryptographicHash::hash(content,QCryptographicHash::Sha256).toHex()),url,content.size()};
        const auto path=Updates::stage(a,cache.path(),cancel,{},true);QVERIFY(QFile::exists(path));const int previous=requests;
        QCOMPARE(Updates::stage(a,cache.path(),cancel,{},true),path);QCOMPARE(requests,previous);
        QFile corrupt(path);QVERIFY(corrupt.open(QIODevice::WriteOnly));corrupt.write("corrupt");corrupt.close();
        QCOMPARE(Updates::stage(a,cache.path(),cancel,{},true),path);QVERIFY(requests>previous);
        QFile::remove(path);a.sha256=QString(64,'f');QVERIFY_EXCEPTION_THROWN(Updates::stage(a,cache.path(),cancel,{},true),std::exception);QVERIFY(!QFile::exists(path));
        auto slow=url;slow.setPath("/slow");QTimer::singleShot(150,this,[&]{cancel=true;});
        QVERIFY_EXCEPTION_THROWN(Updates::fetch(slow,1024,cancel,true),std::exception);
        QCOMPARE(QDir(cache.path()).entryList(QDir::Files|QDir::Hidden|QDir::NoDotAndDotDot).size(),0);
    }
};
QTEST_GUILESS_MAIN(UpdaterTests)
#include "tests.moc"
