#pragma once
#include <QDateTime>
#include <QSettings>
#include <QUrl>
#include <QVariant>

inline bool webAddress(const QUrl &url) {
    return url.isValid() && !url.host().isEmpty() && (url.scheme() == "https" || url.scheme() == "http");
}
inline QUrl withoutCredentials(QUrl url) {
    url.setUserName({});
    url.setPassword({});
    return url;
}
inline void recordVisit(QSettings &settings, bool privateMode, const QUrl &url, const QString &title) {
    if (privateMode || !webAddress(url)) return;
    auto history = settings.value("history").toList();
    const auto clean = withoutCredentials(url).toString();
    for (qsizetype i = history.size(); i-- > 0;)
        if (history[i].toMap().value("url").toString() == clean) history.removeAt(i);
    history.prepend(QVariantMap{{"url", clean}, {"title", title}, {"time", QDateTime::currentDateTimeUtc()}});
    while (history.size() > 200) history.removeLast();
    settings.setValue("history", history);
}
inline QStringList savedTabs(QSettings &settings, bool privateMode) {
    if (privateMode || !settings.value("restoreSession", false).toBool()) return {};
    QStringList result;
    for (const auto &text : settings.value("session").toStringList())
        if (result.size() < 30 && webAddress(QUrl(text))) result.append(withoutCredentials(QUrl(text)).toString());
    return result;
}
