#include "release.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <iostream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        if (argc != 5) throw std::runtime_error("Usage: synterra-update-manifest PACKAGE OUTPUT VERSION BUILD");
        Updates::Plan plan; plan.version = Updates::Version::parse(argv[3], QString(argv[4]).toInt());
        plan.tag = "v" + plan.version.text + "." + QString::number(plan.version.build);
        const auto name = "synterra-release-" + Updates::packageVersion(plan) + "-x86_64.pkg.tar.zst";
        if (QFileInfo(argv[1]).fileName() != name) throw std::runtime_error("Unexpected package filename.");
        Updates::inspectPackage(argv[1], plan);
        std::atomic_bool cancel = false;
        QJsonObject manifest{{"schema", 1}, {"version", plan.version.text}, {"build", plan.version.build}, {"tag", plan.tag},
            {"architecture", "x86_64"}, {"minimum_version", "1.1"},
            {"package", QJsonObject{{"name", name}, {"size", QFileInfo(argv[1]).size()}, {"sha256", QString::fromLatin1(Updates::hashFile(argv[1], cancel))}}}};
        QFile output(argv[2]); if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(manifest).toJson()) < 0) throw std::runtime_error("Cannot write the update manifest.");
        std::cout << "Created " << argv[2] << '\n'; return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
