# Synaqua 0.2

Synaqua is Synterra's new desktop shell, first included in Synterra 1.1 Build 1165 (Prism). Its floating glass dock, centered Start button and pinned apps follow the supplied desktop sketch. The two original Prism wallpapers remain unchanged.

The current version provides an original C++/Qt Quick dock and application launcher. KWin provides Wayland compositing and window management; Plasma supplies the session, desktop folder view, screen locking and logout dialog. These upstream components keep their own licenses. Synaqua supplies the dock and launcher while these components supply desktop services.

## Use it

The live image starts **Synaqua** directly on the desktop. Open Synterra Hub from its pinned dock icon; the welcome-on-login checkbox opts into opening Hub automatically. On the installed system, select it from the desktop-session selector on the sign-in screen. **Plasma (Wayland)** remains available as a fallback.

- Click the center **Start** button or press **Meta+Space** to search installed applications. Enter launches the first search result; arrow keys navigate the grid. Escape or clicking outside closes Start.
- Click the star beside an app to pin or unpin it. Up to eight pinned apps are arranged around Start. Pins are saved in `~/.config/Synterra/Synaqua.conf`.
- Running windows appear in the dock's left-hand strip. Click a window to activate it, or click the active window to minimize it. Scroll horizontally when the strip fills.
- **Home**, **Dark glass/Light glass**, **Lock** and **Power** are in Start. Power opens the existing session confirmation dialog.
- The dock's network and clock buttons open their matching settings pages. The last button toggles Show Desktop.
- Desktop shortcuts provide Home and Synterra Surf; the live image also has Install Synterra.

Synaqua's desktop layout uses `plasma-org.synterra.synaqua-appletsrc`, independently of the normal Plasma layout. Changing Glass mode retains the layout and wallpaper.

## Dock settings

Open **System Settings → Appearance & Style → Synaqua Dock**, choose **Dock settings** in Start, right-click an empty part of the dock, or open its card in Synterra Hub. Apply changes without signing out.

- **Dock size:** 60–125%; shrinks or enlarges the dock, icons and controls together.
- **Dock width:** 45–100% of the available screen width; a small side margin keeps the dock floating.
- **Icon spacing:** 0–16 pixels, scaled with dock size.
- **Distance from screen edge:** 0–48 pixels.
- **Visibility:** clock/date, Synaqua branding and running-window controls.
- **Presets:** Compact, Balanced and Wide. Restore Defaults returns to Balanced after Apply.

Start stays centered. Narrow layouts automatically expand enough to fit pinned apps and controls; on smaller displays oversized choices scale down to fit. The space reserved for maximized windows follows the dock's actual height and edge distance. Existing pins, wallpaper and Glass mode are retained.

Settings are saved per user in `~/.config/Synterra/Synaqua.conf`. `synaqua-shell --settings` opens the same controls as a standalone dialog. Editing controls leaves the current dock unchanged until Apply; closing discards unapplied edits. Settings can be configured from Plasma, then take effect when Synaqua starts.

## Current limits

The dock lives on the primary display. Desktop views on other displays are provided by Plasma. Synaqua does not yet implement a complete status-notifier tray, notification center, dock drag-and-drop, per-screen docks or its own settings/control-center application. Network and clock buttons are settings shortcuts, not live status indicators. Start targets screens of at least 1024×768; accessibility, display hot-plugging and unusual scaling need broader testing.

If the dock needs restarting, run `systemctl --user restart synaqua-shell.service`. If Synaqua cannot start, sign out and select Plasma (Wayland) at the sign-in screen. The shell service restarts on a crash with a bounded retry limit.

## Development

Source lives in `synaqua/`. Its native application catalog uses KService and KIO desktop-file launching, including desktop visibility, TryExec, terminal handling and field-code processing. User input is not interpolated into shell commands. LayerShellQt reserves bottom-screen space and gives only visible shell controls an input region while Start is closed. KWin supplies the glass blur; an open Start menu captures outside clicks for dismissal.

The dock defaults to Qt Quick's software scene graph so the dock also renders on basic virtual graphics. This is local to the shell process; launched applications retain their own renderer choice. KWin still supplies compositor effects. An explicit `QT_QUICK_BACKEND` overrides the default for development.

Build on Arch with Qt 6 Quick/QML/Widgets/DBus, KService, KIO, KGlobalAccel, KWindowSystem, KCMUtils, KCoreAddons, Kirigami and LayerShellQt installed:

```bash
cmake -S synaqua -B target/synaqua -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build target/synaqua --parallel 2
target/synaqua/synaqua-shell --preview
target/synaqua/synaqua-shell --self-test
```

Preview and self-test run as ordinary windows. The self-test uses separate SynaquaTest settings. The regular shell requires a layer-shell Wayland compositor and its installed desktop/session assets. ISO generation copies the installed Plasma shell views and their copyright notices into the separately identified Synaqua desktop-services package; the native dock and Start menu are Synterra code.
