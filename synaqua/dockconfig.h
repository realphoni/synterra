#pragma once
#include <QSettings>
#include <algorithm>

struct DockPreferences {
    int scale = 100, width = 85, spacing = 3, margin = 18;
    bool clock = true, branding = true, windows = true;
    bool operator==(const DockPreferences &other) const {
        return scale == other.scale && width == other.width && spacing == other.spacing && margin == other.margin
            && clock == other.clock && branding == other.branding && windows == other.windows;
    }
    bool operator!=(const DockPreferences &other) const { return !(*this == other); }
    static DockPreferences read(QSettings &settings) {
        auto number = [&](const char *key, int fallback, int low, int high) {
            bool valid = false; const int value = settings.value(key, fallback).toInt(&valid);
            return valid ? std::clamp(value, low, high) : fallback;
        };
        DockPreferences result;
        result.scale = number("dock/scale", 100, 60, 125);
        result.width = number("dock/width", 85, 45, 100);
        result.spacing = number("dock/spacing", 3, 0, 16);
        result.margin = number("dock/margin", 18, 0, 48);
        result.clock = settings.value("dock/showClock", true).toBool();
        result.branding = settings.value("dock/showBranding", true).toBool();
        result.windows = settings.value("dock/showWindows", true).toBool();
        return result;
    }
    void write(QSettings &settings) const {
        settings.setValue("dock/scale", std::clamp(scale, 60, 125));
        settings.setValue("dock/width", std::clamp(width, 45, 100));
        settings.setValue("dock/spacing", std::clamp(spacing, 0, 16));
        settings.setValue("dock/margin", std::clamp(margin, 0, 48));
        settings.setValue("dock/showClock", clock);
        settings.setValue("dock/showBranding", branding);
        settings.setValue("dock/showWindows", windows);
        settings.sync();
    }
};
