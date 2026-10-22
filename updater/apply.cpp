#include "release.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
static int command(const QString &program, const QStringList &args) {
    QProcess process; process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setInputChannelMode(QProcess::ForwardedInputChannel); process.start(program, args);
    require(process.waitForStarted(), "Cannot start the package manager.");
    require(process.waitForFinished(-1), "The package manager did not finish.");
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : 1;
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 4 && QString(argv[1]) == "--apply", "Usage: synterra-release-update --apply RELEASE_TAG DOWNLOADED_PACKAGE");
        require(geteuid() == 0 && isatty(STDIN_FILENO), "Run the update from Synterra Updates in an administrator-assisted terminal.");
        qputenv("PATH", "/usr/bin:/bin");
        std::atomic_bool cancel = false; auto system = Updates::installedSystem();
        require(!system.live, "Release upgrades are disabled in the live environment.");
        QLockFile lock("/run/lock/synterra-release-update.lock"); lock.setStaleLockTime(0);
        require(lock.tryLock(), "Another Synterra release update is running.");
        std::cout << "Rechecking the approved release with GitHub...\n" << std::flush;
        const auto selected = QString::fromLocal8Bit(argv[2]);
        require(QRegularExpression("^v[0-9]{1,6}\\.[0-9]{1,6}\\.[0-9]{1,6}$").match(selected).hasMatch(), "Invalid release tag.");
        auto release = Updates::parseObject(Updates::fetch(QUrl(Updates::Api + "tags/" + selected), 4 * 1024 * 1024, cancel));
        require(release.value("tag_name") == selected, "GitHub returned a different release.");
        auto plan = Updates::resolve(release, system, cancel);
        require(system.version < plan.version, "This release is already installed or would downgrade Synterra.");
        QTemporaryDir quarantine("/var/tmp/synterra-update-XXXXXX"); require(quarantine.isValid(), "Cannot create a private update directory.");
        const auto destination = quarantine.path() + "/" + plan.package.name;
        const int descriptor = open(argv[3], O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
        require(descriptor >= 0, "Cannot open the downloaded update safely.");
        QFile source; require(source.open(descriptor, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle), "Cannot read the update.");
        struct stat details{};
        require(fstat(descriptor, &details) == 0 && S_ISREG(details.st_mode) && details.st_size == plan.package.size, "The downloaded update is not the expected regular file.");
        QFile output(destination); require(output.open(QIODevice::WriteOnly | QIODevice::NewOnly), "Cannot stage the approved update.");
        QCryptographicHash hash(QCryptographicHash::Sha256); qint64 total = 0;
        while (!source.atEnd()) {
            auto data = source.read(1024 * 1024); require(!data.isEmpty() || source.error() == QFile::NoError, "Cannot read the update.");
            total += data.size(); require(total <= plan.package.size, "The downloaded update changed during verification.");
            require(output.write(data) == data.size(), "Cannot stage the update; check disk space."); hash.addData(data);
        }
        require(output.flush(), "Cannot flush the update to disk."); output.close(); source.close();
        require(total == plan.package.size && hash.result().toHex() == plan.package.sha256.toLatin1(), "Update checksum failed. No packages were changed.");
        const auto files = Updates::inspectPackage(destination, plan); QStringList overwrite;
        for (const auto &file : files) {
            const auto target = '/' + file;
            QString current;
            for (const auto &part : file.split('/')) { current += '/' + part; require(!QFileInfo(current).isSymLink(), "A managed update path is symlinked. Resolve it before updating."); }
            if (!QFileInfo::exists(target)) continue;
            QProcess owner; owner.start("/usr/bin/pacman", {"-Qqo", target}); require(owner.waitForFinished(), "Could not check package ownership.");
            const auto package = QString::fromUtf8(owner.readAllStandardOutput()).trimmed();
            require(owner.exitStatus() == QProcess::NormalExit, "Package ownership lookup failed.");
            require(owner.exitCode() != 0 || package == "synterra-release", "An update file belongs to another package. No package files were overwritten.");
            if (owner.exitCode() != 0) overwrite << "--overwrite" << file;
        }
        std::cout << "\nVerified Synterra " << plan.version.text.toStdString() << " Build " << plan.version.build
                  << ".\nYour files and personal desktop settings are retained.\nReview both package transactions below; decline either prompt to stop.\n\n" << std::flush;
        require(command("/usr/bin/pacman", {"-Syu"}) == 0, "The Arch package transaction did not complete. The Synterra release package was not installed.");
        QStringList args{"-U"}; args << overwrite << destination;
        require(command("/usr/bin/pacman", args) == 0, "The release package transaction did not complete. Review pacman's output before retrying.");
        const auto installed = Updates::installedSystem().version;
        require(installed.numbers == plan.version.numbers && installed.build == plan.version.build, "The release package installed, but system branding did not refresh. Review the package hook output before rebooting.");
        if (QFileInfo::exists("/boot/grub/grub.cfg"))
            require(command("/usr/local/libexec/synterra-grub-setup", {}) == 0, "The release package installed, but the GRUB artwork could not be refreshed. Check /boot and rerun sudo synterra-grub-setup.");
        std::cout << "\nSynterra release update finished. Reboot when convenient to load the updated desktop and kernel.\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << "Update stopped: " << error.what() << '\n'; return 1; }
}
