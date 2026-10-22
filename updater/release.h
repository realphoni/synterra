#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <atomic>
#include <functional>
#include <array>

namespace Updates {
inline constexpr qint64 MaxPackage = 512LL * 1024 * 1024;
inline const QString Api = "https://api.github.com/repos/realphoni/synterra/releases/";
struct Version {
    std::array<int, 3> numbers{};
    int build = 0;
    QString text;
    static Version parse(const QString &, int);
    bool operator<(const Version &other) const;
};
struct System { Version version; bool live = false; };
struct Asset { QString name, sha256; QUrl url; qint64 size = 0; };
struct Plan { Version version; QString tag, title, notes; QUrl page; Asset package; };
using Progress = std::function<void(qint64, qint64)>;
System installedSystem();
QJsonObject parseObject(const QByteArray &);
bool allowedUrl(const QUrl &, bool fixture = false);
Asset asset(const QJsonObject &, const QString &tag, const QString &name);
Plan parsePlan(const QJsonObject &release, const QByteArray &manifest, const System &);
QByteArray fetch(const QUrl &, qint64 limit, std::atomic_bool &, bool fixture = false);
QJsonObject latest(std::atomic_bool &);
Plan resolve(const QJsonObject &, const System &, std::atomic_bool &);
QString stage(const Asset &, const QString &directory, std::atomic_bool &, Progress = {}, bool fixture = false);
QByteArray hashFile(const QString &, std::atomic_bool &);
QStringList inspectPackage(const QString &, const Plan &);
bool managedPath(const QString &);
QString packageVersion(const Plan &);
}
