#pragma once
#include <QApplication>
#include <QFile>
#include <QFileSystemWatcher>
#include <QDir>
#include <QPalette>
#include <QIcon>
#include <QStandardPaths>
#include <QTimer>
#include <functional>

namespace GlassAppearance {
inline QString setting(const QString &group, const QString &key) {
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals");
    if (!file.open(QIODevice::ReadOnly)) return {};
    QString section;
    for (const auto &line : QString::fromUtf8(file.readAll()).split('\n')) {
        const auto text = line.trimmed();
        if (text.startsWith('[') && text.endsWith(']')) section = text.mid(1, text.size() - 2);
        else if (section == group && text.startsWith(key + '=')) return text.mid(key.size() + 1);
    }
    return {};
}
inline bool dark() { const auto scheme = setting("General", "ColorScheme"); return scheme == "SynterraGlassDark" || (scheme.isEmpty() && QApplication::palette().color(QPalette::Window).lightness() < 128); }
inline void syncPalette() {
    const auto scheme = setting("General", "ColorScheme");
    if (scheme != "SynterraGlass" && scheme != "SynterraGlassDark") return;
    const bool night = scheme == "SynterraGlassDark";
    QIcon::setThemeName(night ? "SynterraGlassDark" : "SynterraGlass");
    QPalette palette = QApplication::palette();
    for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const auto text = group == QPalette::Disabled ? (night ? "#96adc5" : "#6c7b8c") : (night ? "#e8f2fd" : "#172535");
        palette.setColor(group, QPalette::Window, QColor(night ? "#182432" : "#eff5fb"));
        palette.setColor(group, QPalette::Base, QColor(night ? "#111b27" : "#ffffff"));
        palette.setColor(group, QPalette::AlternateBase, QColor(night ? "#1c2b3b" : "#e7eff7"));
        palette.setColor(group, QPalette::Button, QColor(night ? "#26394d" : "#e6f0f9"));
        for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::ToolTipText}) palette.setColor(group, role, QColor(text));
        palette.setColor(group, QPalette::ToolTipBase, QColor(night ? "#1f3044" : "#f2f8ff"));
        palette.setColor(group, QPalette::Highlight, QColor(night ? "#2c6aa3" : "#3a7ec2"));
        palette.setColor(group, QPalette::HighlightedText, QColor("#ffffff"));
        palette.setColor(group, QPalette::Link, QColor(night ? "#7ac5ff" : "#1f66af"));
    }
    QApplication::setPalette(palette);
}
inline void watch(QObject *owner, std::function<void()> changed) {
    auto watcher = new QFileSystemWatcher(owner);
    auto timer = new QTimer(owner); timer->setSingleShot(true); timer->setInterval(100);
    const auto root = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir().mkpath(root); watcher->addPath(root);
    if (QFile::exists(root + "/kdeglobals")) watcher->addPath(root + "/kdeglobals");
    auto queue = [timer](const QString &) { timer->start(); };
    QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, owner, queue);
    QObject::connect(watcher, &QFileSystemWatcher::fileChanged, owner, queue);
    QObject::connect(timer, &QTimer::timeout, owner, [watcher, root, changed] {
        const auto file = root + "/kdeglobals";
        if (QFile::exists(file) && !watcher->files().contains(file)) watcher->addPath(file);
        syncPalette(); changed();
    });
}
}
