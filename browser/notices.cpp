#include "notices.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <algorithm>

namespace SurfNotices {
namespace {
QString error;
QString normalized(const QString &text) {
    return text.normalized(QString::NormalizationForm_KC).toCaseFolded().simplified();
}
bool domainMatches(const QString &host, const QString &domain) {
    return host == domain || host.endsWith('.' + domain);
}
bool termMatches(const QString &query, const QString &term) {
    // Unicode boundaries prevent x in xbox, steam in steampunk and scp in scp123.
    const QRegularExpression expression("(?<![\\p{L}\\p{N}_])" + QRegularExpression::escape(normalized(term)) + "(?![\\p{L}\\p{N}_])");
    return expression.match(query).hasMatch();
}
QVector<Notice> load() {
    QFile file(":/surf/notices.json");
    if (!file.open(QIODevice::ReadOnly)) { error = "Cannot read the built-in notice catalog."; return {}; }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isArray()) { error = "Invalid built-in notice JSON."; return {}; }
    QVector<Notice> entries;
    QSet<QString> ids;
    for (const auto value : document.array()) {
        const auto object = value.toObject();
        Notice entry;
        entry.id = object["id"].toString(); entry.title = object["title"].toString();
        entry.message = object["message"].toString(); entry.rareMessage = object["rareMessage"].toString();
        const auto tone = object["type"].toString();
        entry.warning = tone == "warning"; entry.playful = object["playful"].toBool();
        for (const auto domain : object["domains"].toArray()) entry.domains.append(domain.toString().toLower());
        for (const auto term : object["terms"].toArray()) entry.terms.append(term.toString());
        entry.source = QUrl(object["source"].toString());
        bool valid = !entry.id.isEmpty() && !ids.contains(entry.id) && !entry.title.isEmpty() &&
            !entry.message.isEmpty() && entry.message.size() <= 500 && entry.rareMessage.size() <= 500 &&
            (tone == "information" || tone == "warning") && (!entry.domains.isEmpty() || !entry.terms.isEmpty());
        for (const auto &domain : entry.domains) valid &= QRegularExpression("^[a-z0-9]+(?:[a-z0-9.-]*[a-z0-9])?\\.[a-z]{2,}$").match(domain).hasMatch() && !domain.contains("..");
        for (const auto &term : entry.terms) valid &= !term.trimmed().isEmpty() && term.size() <= 100;
        if (!entry.source.isEmpty()) valid &= entry.source.isValid() && entry.source.scheme() == "https" && !entry.source.host().isEmpty() && entry.source.userInfo().isEmpty();
        if (!valid) { error = "Invalid or duplicate built-in notice: " + entry.id; return {}; }
        ids.insert(entry.id); entries.append(entry);
    }
    if (entries.isEmpty()) error = "The built-in notice catalog is empty.";
    return entries;
}
}
const QVector<Notice> &catalog() { static const auto entries = load(); return entries; }
QString catalogError() { catalog(); return error; }
QString searchTerms(const QUrl &url) {
    if (url.scheme() != "https" && url.scheme() != "http") return {};
    const auto host = url.host().toLower(), path = url.path();
    QString key;
    if ((host == "duckduckgo.com" || host == "www.duckduckgo.com" || host == "html.duckduckgo.com") &&
        (path.isEmpty() || path == "/" || path == "/html" || path == "/html/")) key = "q";
    const QStringList google{"google.com", "www.google.com", "google.de", "www.google.de", "google.co.uk", "www.google.co.uk"};
    if (google.contains(host) && path == "/search") key = "q";
    if ((host == "bing.com" || host == "www.bing.com" || host == "search.brave.com") && path == "/search") key = "q";
    if ((host == "youtube.com" || host == "www.youtube.com" || host == "m.youtube.com") && path == "/results") key = "search_query";
    if (key.isEmpty()) return {};
    const auto values = QUrlQuery(url).allQueryItemValues(key, QUrl::FullyEncoded);
    if (values.size() != 1 || values.first().size() > 2048) return {};
    const auto text = QUrl::fromPercentEncoding(QString(values.first()).replace('+', ' ').toUtf8());
    return text.size() <= 256 ? normalized(text) : QString();
}
QVector<Match> match(const QUrl &url, bool searchTopics, bool playful) {
    QVector<Match> matches;
    if (!url.isValid() || (url.scheme() != "https" && url.scheme() != "http")) return matches;
    const auto host = url.host().toLower();
    const auto query = searchTopics ? searchTerms(url) : QString();
    for (const auto &entry : catalog()) {
        if (entry.playful && !playful) continue;
        const bool site = std::any_of(entry.domains.begin(), entry.domains.end(), [&](const QString &domain) { return domainMatches(host, domain); });
        const bool topic = !query.isEmpty() && std::any_of(entry.terms.begin(), entry.terms.end(), [&](const QString &term) { return termMatches(query, term); });
        if (site || topic) matches.append({entry, !site});
    }
    std::stable_sort(matches.begin(), matches.end(), [](const Match &a, const Match &b) { return a.notice.warning > b.notice.warning; });
    return matches;
}
QString message(const Notice &notice, quint32 variant) {
    return !notice.rareMessage.isEmpty() && variant % 20 == 0 ? notice.rareMessage : notice.message;
}
QVector<Match> visible(const QVector<Match> &matches, const TabState &state) {
    QVector<Match> result;
    for (const auto &entry : matches) if (!state.dismissed.contains(entry.notice.id)) result.append(entry);
    return result;
}
}
