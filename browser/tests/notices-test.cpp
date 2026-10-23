#include "../notices.h"
#include <QTest>
#include <QUrlQuery>

static QStringList ids(const QUrl &url, bool searches = true, bool playful = true) {
    QStringList result;
    for (const auto &entry : SurfNotices::match(url, searches, playful)) result.append(entry.notice.id);
    return result;
}
class NoticesTest : public QObject {
    Q_OBJECT
private slots:
    void catalogValid() {
        QVERIFY2(SurfNotices::catalogError().isEmpty(), qPrintable(SurfNotices::catalogError()));
        QCOMPARE(SurfNotices::catalog().size(), 21);
        for (const auto &entry : SurfNotices::catalog()) {
            if (entry.warning) QVERIFY(!entry.source.isEmpty());
            QVERIFY(!entry.id.isEmpty());
        }
    }
    void domains_data() {
        QTest::addColumn<QString>("url"); QTest::addColumn<QStringList>("expected");
        QTest::newRow("site") << "https://www.roblox.com/games/42" << QStringList{"roblox"};
        QTest::newRow("subdomain") << "https://en.help.roblox.com/" << QStringList{"roblox"};
        QTest::newRow("case") << "https://STORE.STEAMPOWERED.COM/" << QStringList{"steam"};
        QTest::newRow("suffix-spoof") << "https://roblox.com.evil.example/" << QStringList{};
        QTest::newRow("prefix-spoof") << "https://notroblox.com/" << QStringList{};
        QTest::newRow("userinfo") << "https://roblox.com@evil.example/" << QStringList{};
        QTest::newRow("path") << "https://example.org/roblox.com" << QStringList{};
        QTest::newRow("fragment") << "https://example.org/#roblox" << QStringList{};
        QTest::newRow("foreign-query") << "https://example.org/?q=roblox" << QStringList{};
        QTest::newRow("unicode-spoof") << QString::fromUtf8("https://rоblox.com/") << QStringList{};
        QTest::newRow("file") << "file:///roblox.com" << QStringList{};
        QTest::newRow("script") << "javascript:roblox" << QStringList{};
        QTest::newRow("home") << "qrc:/surf/start.html" << QStringList{};
        QTest::newRow("real-x") << "https://x.com/" << QStringList{"x"};
        QTest::newRow("twitter") << "https://twitter.com/" << QStringList{"twitter"};
    }
    void domains() { QFETCH(QString, url); QFETCH(QStringList, expected); QCOMPARE(ids(QUrl(url)), expected); }
    void queries_data() {
        QTest::addColumn<QString>("url"); QTest::addColumn<QStringList>("expected");
        QTest::newRow("duckduckgo") << "https://duckduckgo.com/?q=ROBLOX+privacy" << QStringList{"roblox"};
        QTest::newRow("google") << "https://www.google.de/search?q=ace%20attorney" << QStringList{"ace-attorney"};
        QTest::newRow("youtube") << "https://www.youtube.com/results?search_query=phoni1rbx" << QStringList{"phoni"};
        QTest::newRow("bing") << "https://www.bing.com/search?q=jarona" << QStringList{"jarona"};
        QTest::newRow("brave") << "https://search.brave.com/search?q=AO3" << QStringList{"ao3"};
        QTest::newRow("space-plus") << "https://duckduckgo.com/?q=phoni+technology" << QStringList{"phoni-technology", "phoni"};
        QTest::newRow("literal-plus") << "https://duckduckgo.com/?q=ace%2Battorney" << QStringList{};
        QTest::newRow("unicode-case") << "https://duckduckgo.com/?q=%EF%BC%B3%EF%BC%A3%EF%BC%B0" << QStringList{"scp"};
        QTest::newRow("word-boundaries") << "https://duckduckgo.com/?q=steampunk+scpx+myroblox+xbox" << QStringList{};
        QTest::newRow("unicode-boundaries") << QString::fromUtf8("https://duckduckgo.com/?q=érobloxé") << QStringList{};
        QTest::newRow("duplicate-query") << "https://duckduckgo.com/?q=roblox&q=steam" << QStringList{};
        QTest::newRow("wrong-path") << "https://www.google.com/maps?q=roblox" << QStringList{};
        QTest::newRow("fake-search-host") << "https://duckduckgo.com.evil.example/?q=roblox" << QStringList{};
        QTest::newRow("double-encoded") << "https://duckduckgo.com/?q=ace%2520attorney" << QStringList{};
    }
    void queries() { QFETCH(QString, url); QFETCH(QStringList, expected); QCOMPARE(ids(QUrl(url)), expected); }
    void priorityAndControls() {
        const QUrl url("https://duckduckgo.com/?q=steam+roblox+ao3");
        QCOMPARE(ids(url), (QStringList{"roblox", "ao3", "steam"}));
        QVERIFY(ids(url, false).isEmpty());
        QCOMPARE(ids(url, true, false), (QStringList{"roblox", "ao3"}));
        QVERIFY(ids(QUrl("https://store.steampowered.com/"), true, false).isEmpty());
        QCOMPARE(ids(QUrl("https://www.roblox.com/"), false, false), QStringList{"roblox"});
        auto matches = SurfNotices::match(url);
        SurfNotices::TabState first, second;
        first.dismissed.insert("roblox");
        QCOMPARE(SurfNotices::visible(matches, first).size(), 2);
        QCOMPARE(SurfNotices::visible(matches, second).size(), 3);
        first.dismissed.clear(); QCOMPARE(SurfNotices::visible(matches, first).size(), 3);
    }
    void longQueriesAndRareCopy() {
        QUrl url("https://duckduckgo.com/"); QUrlQuery query;
        query.addQueryItem("q", "roblox " + QString(300, 'a')); url.setQuery(query);
        QVERIFY(ids(url).isEmpty());
        auto ace = SurfNotices::match(QUrl("https://duckduckgo.com/?q=ace+attorney")).first().notice;
        QCOMPARE(SurfNotices::message(ace, 0), ace.rareMessage);
        QCOMPARE(SurfNotices::message(ace, 1), ace.message);
        QCOMPARE(SurfNotices::message(ace, 20), ace.rareMessage);
    }
};
QTEST_GUILESS_MAIN(NoticesTest)
#include "notices-test.moc"
