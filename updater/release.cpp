#include "release.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStorageInfo>
#include <QTimer>
#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>
#include <memory>
#include <algorithm>

namespace Updates {
static void require(bool ok, const QString &message) { if (!ok) throw std::runtime_error(message.toStdString()); }
Version Version::parse(const QString &text, int build) {
    auto match = QRegularExpression("^(0|[1-9][0-9]{0,5})\\.(0|[1-9][0-9]{0,5})(?:\\.(0|[1-9][0-9]{0,5}))?$").match(text);
    require(match.hasMatch() && build > 0, "Invalid Synterra version or build.");
    return {{{match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()}}, build, text};
}
bool Version::operator<(const Version &other) const { return numbers != other.numbers ? numbers < other.numbers : build < other.build; }
System installedSystem() {
    QFile file("/etc/os-release"); require(file.open(QIODevice::ReadOnly), "Cannot read the installed system version.");
    QMap<QString, QString> fields;
    for (const auto &line : file.readAll().split('\n')) {
        auto eq = line.indexOf('='); if (eq < 1) continue;
        auto value = QString::fromUtf8(line.mid(eq + 1)).trimmed();
        if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.size() - 2);
        fields[QString::fromUtf8(line.left(eq))] = value;
    }
    require(fields.value("ID") == "synterra", "Release upgrades are available on installed Synterra systems.");
    return {Version::parse(fields.value("VERSION_ID"), fields.value("BUILD_ID").toInt()), QDir("/run/archiso").exists()};
}
QJsonObject parseObject(const QByteArray &data) {
    QJsonParseError error; auto doc = QJsonDocument::fromJson(data, &error);
    require(error.error == QJsonParseError::NoError && doc.isObject(), "The release server returned invalid JSON."); return doc.object();
}
bool allowedUrl(const QUrl &url, bool fixture) {
    if (fixture && url.scheme() == "http" && url.host() == "127.0.0.1" && url.userInfo().isEmpty()) return true;
    return url.scheme() == "https" && url.userInfo().isEmpty() && (url.port(-1) == -1 || url.port() == 443) &&
        QStringList{"api.github.com", "github.com", "release-assets.githubusercontent.com", "objects.githubusercontent.com"}.contains(url.host());
}
static bool digest(const QString &value) { return QRegularExpression("^[0-9a-f]{64}$").match(value).hasMatch(); }
static void tagValid(const QString &tag) { require(QRegularExpression("^v[0-9]{1,6}\\.[0-9]{1,6}(?:\\.[0-9]{1,6})?$").match(tag).hasMatch(), "Unsupported release tag."); }
Asset asset(const QJsonObject &release, const QString &tag, const QString &name) {
    tagValid(tag); Asset result; int count = 0;
    for (const auto &value : release.value("assets").toArray()) {
        auto item = value.toObject(); if (item.value("name").toString() != name) continue; ++count;
        result = {name, item.value("digest").toString().mid(7), QUrl(item.value("browser_download_url").toString()), item.value("size").toInteger(-1)};
        require(item.value("state") == "uploaded" && item.value("digest").toString().startsWith("sha256:"), "Release download is incomplete or has no checksum.");
    }
    require(count == 1 && digest(result.sha256) && result.size > 0 && result.size <= MaxPackage, "A compatible, verified Synterra update is not attached to this release.");
    const QString path = "/realphoni/synterra/releases/download/" + tag + "/" + name;
    require(result.url.host() == "github.com" && result.url.path() == path && result.url.query().isEmpty() && result.url.fragment().isEmpty() && allowedUrl(result.url), "Unexpected release download address.");
    return result;
}
QString packageVersion(const Plan &plan) { return plan.version.text + "." + QString::number(plan.version.build) + "-1"; }
Plan parsePlan(const QJsonObject &release, const QByteArray &bytes, const System &system) {
    require(release.value("draft").isBool() && !release.value("draft").toBool() && release.value("prerelease").isBool() && !release.value("prerelease").toBool(), "Only published full releases can be installed.");
    auto manifest = parseObject(bytes); Plan plan;
    plan.tag = release.value("tag_name").toString(); tagValid(plan.tag);
    plan.version = Version::parse(manifest.value("version").toString(), manifest.value("build").toInt());
    require(manifest.value("schema").toInt() == 1 && manifest.value("tag") == plan.tag && manifest.value("architecture") == "x86_64", "Unsupported update manifest.");
    require(plan.tag == "v" + plan.version.text + "." + QString::number(plan.version.build), "Release tag and update version disagree.");
    auto minimum = Version::parse(manifest.value("minimum_version").toString(), 1);
    require(!(system.version < minimum), "This release requires a newer Synterra base. See its release notes for migration instructions.");
    auto package = manifest.value("package").toObject();
    const auto name = "synterra-release-" + packageVersion(plan) + "-x86_64.pkg.tar.zst";
    require(package.value("name") == name, "Unexpected update package name.");
    plan.package = asset(release, plan.tag, name);
    require(package.value("sha256") == plan.package.sha256 && package.value("size").toInteger() == plan.package.size, "The update manifest and release checksum disagree.");
    plan.page = QUrl("https://github.com/realphoni/synterra/releases/tag/" + plan.tag);
    plan.title = release.value("name").toString().left(200); plan.notes = release.value("body").toString().left(32000);
    return plan;
}
static void transfer(const QUrl &url, qint64 size, QIODevice &output, std::atomic_bool &cancel, Progress progress, bool fixture) {
    require(allowedUrl(url, fixture), "Refusing an untrusted update address.");
    require(!cancel, "Download cancelled.");
    QNetworkAccessManager manager; QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Synterra-Updates/2.0.2010");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);
    request.setTransferTimeout(30000); request.setMaximumRedirectsAllowed(5);
    auto reply = manager.get(request); reply->setReadBufferSize(256 * 1024);
    QEventLoop loop; QTimer tick, deadline; tick.setInterval(100); deadline.setSingleShot(true); deadline.start(30 * 60 * 1000);
    QString failure; qint64 received = 0;
    auto drain = [&] {
        while (reply->bytesAvailable() > 0 && failure.isEmpty()) {
            auto chunk = reply->read(64 * 1024); received += chunk.size();
            if (received > size) failure = "The download exceeded its declared size.";
            else if (output.write(chunk) != chunk.size()) failure = "Could not save the update. Check available disk space.";
        }
        if (!failure.isEmpty()) reply->abort();
        if (progress) progress(received, size);
    };
    QObject::connect(reply, &QNetworkReply::redirected, &loop, [&](const QUrl &next) {
        if (allowedUrl(next, fixture)) reply->redirectAllowed(); else { failure = "The download redirected to an untrusted address."; reply->abort(); }
    });
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, drain);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] { failure = "The download timed out. Please try again."; reply->abort(); });
    QObject::connect(&tick, &QTimer::timeout, &loop, [&] { if (cancel) { failure = "Download cancelled."; reply->abort(); } });
    tick.start(); loop.exec(); drain();
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    require(failure.isEmpty(), failure);
    require(reply->error() == QNetworkReply::NoError && status == 200, status == 403 || status == 429 ? "GitHub's request limit was reached. Please try again later." : "Could not download the release: " + reply->errorString());
}
QByteArray fetch(const QUrl &url, qint64 limit, std::atomic_bool &cancel, bool fixture) {
    QBuffer buffer; buffer.open(QIODevice::WriteOnly); transfer(url, limit, buffer, cancel, {}, fixture); return buffer.data();
}
QJsonObject latest(std::atomic_bool &cancel) { return parseObject(fetch(QUrl(Api + "latest"), 4 * 1024 * 1024, cancel)); }
Plan resolve(const QJsonObject &release, const System &system, std::atomic_bool &cancel) {
    const auto tag = release.value("tag_name").toString(); auto manifest = asset(release, tag, "synterra-update.json");
    require(manifest.size <= 65536, "The update manifest is too large.");
    auto data = fetch(manifest.url, manifest.size, cancel);
    require(data.size() == manifest.size && QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() == manifest.sha256.toLatin1(), "Update manifest checksum failed.");
    return parsePlan(release, data, system);
}
QByteArray hashFile(const QString &path, std::atomic_bool &cancel) {
    QFile file(path); require(file.open(QIODevice::ReadOnly), "Cannot read the downloaded update.");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) { require(!cancel, "Download cancelled."); auto data = file.read(1024 * 1024); require(!data.isEmpty() || file.error() == QFile::NoError, "Cannot read the downloaded update."); hash.addData(data); }
    return hash.result().toHex();
}
QString stage(const Asset &asset, const QString &directory, std::atomic_bool &cancel, Progress progress, bool fixture) {
    require(QDir().mkpath(directory), "Cannot create the update cache.");
    require(!QFileInfo(directory).isSymLink(), "Refusing a symlinked update cache.");
    const auto path = directory + "/" + asset.name;
    if (QFileInfo(path).isFile() && !QFileInfo(path).isSymLink() && QFileInfo(path).size() == asset.size && hashFile(path, cancel) == asset.sha256.toLatin1()) return path;
    require(QStorageInfo(directory).bytesAvailable() > asset.size + 16 * 1024 * 1024, "Not enough free space to download the update.");
    QSaveFile output(path); require(output.open(QIODevice::WriteOnly), "Cannot save the update.");
    transfer(asset.url, asset.size, output, cancel, progress, fixture);
    require(output.size() == asset.size, "The update download was interrupted.");
    require(output.commit(), "Cannot finish saving the update.");
    if (hashFile(path, cancel) != asset.sha256.toLatin1()) { QFile::remove(path); throw std::runtime_error("Update checksum failed. The download was discarded."); }
    return path;
}
bool managedPath(const QString &path) {
    const QStringList prefixes = {"etc/skel/", "etc/systemd/user/plasma-plasmashell.service.d/", "usr/share/synterra/", "usr/share/plasma/shells/org.synterra.synaqua/", "usr/share/plasma/look-and-feel/org.synterra.glass.", "usr/share/plasma/desktoptheme/SynterraGlass", "usr/share/aurorae/themes/SynterraGlass", "usr/share/icons/SynterraGlass", "usr/share/sddm/themes/SynterraGlass/", "usr/share/wallpapers/Synterra-", "usr/share/grub/themes/SynterraPrism/"};
    if (path.startsWith('/') || path.contains("..") || path.contains(',') || path.contains('*') || path.contains('?') || path.contains('[') || path.contains('\\')) return false;
    for (const auto &prefix : prefixes) if (path.startsWith(prefix)) return true;
    return QRegularExpression("^(usr/local/(bin|libexec)/synterra-[a-z-]+|usr/local/(bin/synaqua-shell|bin/start-synaqua|libexec/synaqua-autostart)|usr/share/applications/(synterra-[a-z-]+|synaqua)\\.desktop|usr/share/icons/hicolor/scalable/apps/synterra(-[a-z-]+)?\\.svg|usr/share/color-schemes/SynterraGlass(Dark)?\\.colors|usr/share/wayland-sessions/synaqua\\.desktop|usr/lib/systemd/user/synaqua-shell\\.service|usr/lib/qt6/plugins/(styles/libSynterraGlass\\.so|plasma/kcms/systemsettings/kcm_synaqua_dock\\.so)|usr/share/libalpm/hooks/90-synterra-branding\\.hook|usr/share/licenses/synterra-release/LICENSE)$").match(path).hasMatch();
}
QStringList inspectPackage(const QString &path, const Plan &plan) {
    auto archive = std::unique_ptr<struct archive, decltype(&archive_read_free)>(archive_read_new(), archive_read_free);
    archive_read_support_filter_zstd(archive.get()); archive_read_support_format_tar(archive.get());
    require(archive_read_open_filename(archive.get(), path.toUtf8().constData(), 64 * 1024) == ARCHIVE_OK, "Cannot read the update package.");
    archive_entry *entry; QSet<QString> seen; QStringList files, directories; QByteArray info; qint64 total = 0; int status;
    while ((status = archive_read_next_header(archive.get(), &entry)) == ARCHIVE_OK) {
        const auto name = QString::fromUtf8(archive_entry_pathname(entry));
        require(!seen.contains(name) && seen.size() < 10000, "Duplicate or excessive package entries."); seen.insert(name);
        require(!name.startsWith('/') && !name.split('/').contains("..") && !name.split('/').contains(".") && !archive_entry_hardlink(entry) && !archive_entry_symlink(entry), "Unsafe update package path or link.");
        auto type = archive_entry_filetype(entry);
        require(type == AE_IFREG || type == AE_IFDIR, "Unsupported update package entry.");
        require(archive_entry_uid(entry) == 0 && archive_entry_gid(entry) == 0, "Release files must be owned by root.");
        require((archive_entry_perm(entry) & 07000) == 0, "Privileged file modes are not permitted in release updates.");
        const auto size = archive_entry_size(entry); require(size >= 0 && size <= MaxPackage && (total += size) <= 2 * MaxPackage, "Update package is too large.");
        if (type == AE_IFDIR) { require(archive_entry_perm(entry) == 0755, "Unexpected update directory permissions."); directories << (name.endsWith('/') ? name : name + '/'); continue; }
        require(archive_entry_perm(entry) == 0644 || archive_entry_perm(entry) == 0755, "Unexpected update file permissions: " + name);
        if (name == ".PKGINFO") { require(size < 65536, "Invalid package metadata."); info.resize(size); require(archive_read_data(archive.get(), info.data(), size) == size, "Cannot read package metadata."); }
        else if (name != ".BUILDINFO" && name != ".MTREE") { require(managedPath(name), "The release package contains an unmanaged file: " + name); files << name; }
    }
    require(status == ARCHIVE_EOF, "The update package is damaged.");
    const auto metadata = QString::fromUtf8(info).split('\n');
    require(metadata.count("pkgname = synterra-release") == 1 && metadata.count("pkgver = " + packageVersion(plan)) == 1 && metadata.count("arch = x86_64") == 1 && !files.isEmpty(), "Package identity does not match the approved release.");
    for (const auto &directory : directories) require(std::any_of(files.begin(), files.end(), [&](const QString &file) { return file.startsWith(directory); }), "The package contains an unrelated directory: " + directory);
    return files;
}
}
