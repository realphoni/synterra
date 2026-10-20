# Build status — 2026-10-20

Synterra 1.1 Build 1170 (Prism) introduces Synaqua 0.2 dock settings. The image retains Build 1165's verified Arch package/kernel base and replaces Synterra's native applications, settings plugin, desktop assets and branding. A normal full Archiso build remains available through the repository's build scripts.

- Image: `synterra-1.1.1170-x86_64.iso`
- Size: 2,595,487,744 bytes
- SHA-256: `a5880a1528629446204475dbdf8412d3d669d65358c696e2d546d4e3c78a5002`
- Release: [v1.1.1170](https://github.com/realphoni/synterra/releases/tag/v1.1.1170), split into two ISO parts with whole-image and part checksums.

## Changes

System Settings → Appearance & Style → Synaqua Dock configures dock size (60–125%), width (45–100%), icon spacing, edge distance, clock/date, branding and running-window visibility. Start, the dock's empty-space right-click and Hub provide entry points; `synaqua-shell --settings` opens a standalone dialog.

Apply persists settings per user without changing pins, wallpaper or Glass mode. Compact/Balanced/Wide presets and defaults are provided. Start stays centered, layouts leave room for up to eight pins and oversized choices fit smaller screens. LayerShellQt's reserved zone and shell input region track the actual dock geometry.

Synaqua continues to use KWin/Plasma services. Surf's feature version remains 1.1.1; this build updates its OS branding.

## Validation

Source validation, installer regressions, Surf self-tests and native compilation passed. The actual shared KDE Settings module loaded through KPluginFactory; Apply, persistence, Reset, Defaults and unchanged pins passed as an unprivileged user. It appears in the installed KCM catalog.

Native Light/Dark UI tests passed for search, Enter activation, dismissal, persistent pins and live external settings reload. Tests cover unsaved edits, compact/wide presets, persisted values read by a second shell instance, malformed/out-of-range settings and visibility toggles. Nine size/width combinations with eight pinned apps fit a 1024×768 display without overlapping the right-side controls or moving Start off center.

The same tests also passed through a real isolated KWin Wayland compositor, including dynamic LayerShellQt exclusion-zone checks. Settings-page and compact/wide screenshots were visually inspected. Native preview screenshots are labeled as previews.

Read-only ISO inspection verified the filesystem checksum, Build 1170 metadata and matching tested shell/Settings binaries in the live and installed payloads. BIOS and UEFI menus were rendered from the final image and visually inspected. The settings page and compact/wide images are native test captures.

## Remaining validation

A complete installation and installed login authentication, encrypted installations, BIOS desktop startup, VMware 3D acceleration, multiple monitors/hot-plugging, accessibility and wider hardware testing remain outstanding. The dock is primary-display only and still has no complete tray or notification center. See the [Synaqua guide](SYNAQUA.md).

Previous build validation is archived in [Build 1165 status](BUILD-STATUS-1165.md).
