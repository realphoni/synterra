# Build status — 2026-10-09

Synterra 1.1 Build 1130 (Prism) built successfully with official Archiso in SynterraBuild.

- Image: synterra-1.1.1130-x86_64.iso.
- Size: 2,412,625,920 bytes (about 2.25 GiB).
- SHA-256: 0f6a9e4e5a994fd4a33c984b9223b45e51816f2c4276b73c76b210d8aaadb950.
- Release: v1.1.1130, distributed in two ISO parts with whole-image and part checksums, a Surf 1.1.1 update archive/checksum and screenshots.

## Changes

This is a browser-focused release. Surf 1.1.1 adds loading progress, Stop/Reload, per-tab muting, a recoverable failed-load/web-process-crash notice and per-tab Chromium developer tools. Page updates preserve address-bar edits; selecting another tab updates its address. Ctrl+Shift+R reloads while bypassing HTTP cache.

User-triggered JavaScript windows open using the original request without racing a Home load. Opener relationships are retained, script-opened tabs can close themselves, and background links leave the current tab selected. Automatic popups stay blocked and display a status message. Developer tools share their window's profile, including the isolated private profile, and are destroyed before the inspected tab/profile.

Light/Dark Glass, Synterra's widget/icon/login themes, Hub, installer recovery, GRUB artwork and the supplied wallpapers remain included. OS, installer, browser, desktop, login and boot branding identify Build 1130. This release does not redesign the desktop or change installation behavior.

## Validation

Rust source validation and installer target/greeter regressions passed. The final Surf and Hub binaries and Glass style plugin compiled. Surf's URL, port/path, scheme rejection, history cap/deduplication, credential stripping and session/private self-tests passed.

The final browser ran as an unprivileged user with Chromium sandboxing enabled, isolated test configuration and a virtual X11 display. Both normal and private runs passed real rendering, find results, tab closure, an HTTP download fixture, fullscreen/Escape, zoom, bookmark search/removal and PDF export. The new HTTP/JavaScript fixture passed async/await, fetch/JSON, DOM and local-storage checks, automatic popup blocking, actual mouse-triggered popup opening, opener/close callbacks and Ctrl-click background tabs. Tab muting stayed isolated; address editing and new-tab address replacement passed. F12 attached the inspector to the correct page/profile, closing it detached it, and closing its tab removed its inspector. Stop did not produce an error; a failed HTTP load and an actual killed renderer recovered on reload.

The finished ISO was mounted read-only. Both firmware boot files and GRUB syntax passed. Live and installed payloads contain Build 1130 branding and the exact tested Surf binary. Executable helpers/plugins, Glass defaults, both themes, login QML and original wallpaper hashes passed inspection. BIOS Syslinux and UEFI GRUB menus rendered from this ISO and were visually inspected. The download parts were streamed together and verified against the whole-image SHA-256 without creating a duplicate ISO.

An isolated QEMU UEFI VM with 3 GiB RAM, no network and no hard disk reached the Plasma desktop, Aurora wallpaper, bottom taskbar and Synterra Hub. Surf launched from the final ISO and rendered its actual homepage with Build 1130 branding and Glass controls. Its initial load was slow under software emulation. Live screenshots were visually inspected; browser behavior regressions above ran natively on the build host.

## Remaining validation

A complete installation to a disposable VM disk, installed login authentication, encrypted installations, BIOS desktop startup, wider hardware compatibility, physical Bluetooth/KDE Connect pairing and broad website/download-cancellation behavior still need testing. JavaScript tests use local fixtures; they do not establish compatibility with every sign-in provider or website. Release signing is future work.

At Archinstall completion choose Exit and wait for Synterra finalization before rebooting. If a guard fails, keep the live session running and inspect the reported condition.
