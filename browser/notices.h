#pragma once

#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QVector>

namespace SurfNotices {
struct Notice {
    QString id, title, message;
    QStringList domains, terms;
    QUrl source;
    bool warning = false;
    bool playful = false;
    QString rareMessage;
};
struct Match {
    Notice notice;
    bool searchTopic = false;
};
const QVector<Notice> &catalog();
QString catalogError();
QString searchTerms(const QUrl &url);
QVector<Match> match(const QUrl &url, bool searchTopics = true, bool playful = true);
QString message(const Notice &notice, quint32 variant);
// Dismissals stay in the owning tab's memory, including in private windows.
struct TabState {
    QSet<QString> dismissed;
    QString context;
    int selected = 0;
};
QVector<Match> visible(const QVector<Match> &matches, const TabState &state);
}
