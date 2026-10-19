# Build status — 2026-10-19

Synterra 1.1 Build 1165 (Prism) was built with official Archiso in SynterraBuild WSL. It introduces Synaqua 0.1, an experimental desktop-shell preview.

- Image: `synterra-1.1.1165-x86_64.iso`
- Size: 2,596,274,176 bytes (about 2.42 GiB)
- SHA-256: `ac025a37c68dcb0ada245595dfbf77b828f38ff53c7a57d7f11f5356e4dfb3b2`
- Release: [v1.1.1165](https://github.com/realphoni/synterra/releases/tag/v1.1.1165), with two ISO parts and whole-image/part checksums.

## Changes

Synaqua adds an original native C++/Qt Quick floating glass dock, centered Start button, persistent application pins on both sides, searchable application catalog and running-window controls. Home, Glass switching, Lock, Power, settings shortcuts, a clock and Show Desktop are included. The live image opens directly on the desktop; Hub remains pinned and can be opted into at login.

KWin provides Wayland compositing/window management. Plasma supplies desktop views, session startup, locking and logout. Synaqua has a separate desktop-services package and layout, while Plasma (Wayland) remains available as a fallback. This is not an independent compositor or a complete KDE replacement. See the [Synaqua guide](SYNAQUA.md) for controls and current limits.

The shell defaults to a local software scene graph for basic virtual graphics without changing launched applications' renderer environment. The image uses Zstandard filesystem compression. Light/Dark Glass, original Aurora/Graphite wallpapers, GRUB artwork, Surf 1.1.1 and Hub remain included with Build 1165 branding.

Installer account checks now cover the upper regular-user UID boundary, normalized home paths and symlinked home ancestors. A mounted existing installation is directed to guarded recovery instead of repartitioning. Finalization selects the installed Synaqua session and retains the created account's authentication. Live-user autologin and passwordless sudo are not copied.

## Validation

Rust source validation and installer regressions passed, including UID/home-path cases and display-manager recovery. Surf, Hub, Glass and Synaqua compiled against current Arch packages. Hub's live/installed action guards and Synaqua welcome behavior passed.

Native Synaqua UI tests passed in Light and Dark modes using its default renderer: application search and empty results, Enter activation, menu visibility, Escape/outside dismissal, centered Start at 1024 pixels, bounded persistent pins and invalid desktop IDs. A real KIO desktop-file launch preserved a quoted argument containing spaces. These checks used isolated test settings and an unprivileged account.

The final shell also ran inside a real isolated KWin Wayland session with Plasma desktop services. Its dock, Start menu and D-Bus activation rendered successfully. Native preview screenshots are labeled separately from live-ISO captures.

The finished ISO's SHA-256 was independently checked on Windows. Read-only ISO inspection and userspace filesystem extraction verified Build 1165 branding, executable helpers, both session payloads and the exact tested Synaqua binary. The installed payload retains Home/Surf shortcuts and excludes the live installer shortcut.

The final ISO booted through UEFI in an isolated QEMU VM with no network or hard disk and reached Synaqua without renderer/debug overrides. The actual desktop, supplied wallpaper, three desktop shortcuts, dock and Start menu were visually inspected. Konsole launched from Start and appeared in the running-window strip. Software emulation is slow; native UI tests provide the more detailed interaction checks.

## Remaining validation

A complete installation to a disposable VM disk, installed login authentication, encrypted installations, BIOS desktop startup, VMware 3D acceleration, multiple displays/hot-plugging, accessibility and broad hardware testing remain outstanding. The dock currently targets the primary display and does not implement a complete tray or notification center. Physical Bluetooth/KDE Connect pairing and broad website compatibility remain untested. Release signing is future work.

At Archinstall completion choose Exit and wait for Synterra finalization before rebooting. If a guard fails, keep the live session running and inspect the reported condition.

Previous browser-focused validation is archived in [Build 1130 status](BUILD-STATUS-1130.md).
