# Build status — 2026-10-07

Synterra 1.1 Build 1105 (Prism) built successfully with official Archiso in `SynterraBuild`.

- Image: `synterra-1.1.1105-x86_64.iso`.
- Size: 2,407,563,264 bytes (about 2.24 GiB).
- SHA-256: `73c8b74c34e8181aafa67277b63aafa8088269c034f782591e1004d96507498b`.
- Release: `v1.1.1105`, distributed in two ISO parts with whole-image and part checksums, a Surf 1.1 update archive/checksum and screenshots.

## Changes

Light and Dark Glass now cover Plasma surfaces, Aurorae decorations and application colors. Synterra Glass replaces Breeze as the default Qt 6 widget style, using original painting for glossy buttons, inputs, tabs, headers and selection controls. Qt Fusion supplies geometry and fallback drawing for remaining controls. Original glass launcher, Surf, folder, terminal, appearance and calculator icons form the active Synterra Glass icon theme; Light/dark Breeze and hicolor supply remaining icons.

Synterra Hub provides a native appearance dialog, backed by `synterra-appearance light|dark`. Hub and Surf follow live palette changes, including atomic configuration replacement. The original Aurora and Graphite wallpapers are unchanged. A new Synterra Glass SDDM login theme includes light/dark cards and routes credentials through SDDM's normal authentication API. The live account still signs in automatically.

OS, installer, browser, desktop and boot branding identify Synterra 1.1 Build 1105, codenamed Prism. Existing Surf, Hub, installer recovery, GRUB splash, Bluetooth and KDE Connect features remain included. Standard Fastfetch remains available without a custom preset or logo.

## Validation

Rust source validation and installer target/greeter regressions passed. Bash and GRUB syntax checks passed. Native Surf, Hub and the Glass style plugin compiled. Real plugin previews passed in both palettes, including text contrast and keyboard/pointer controls. The live palette watcher passed atomic light-to-dark configuration replacement. Sandboxed unprivileged Surf tests passed rendering, find results, tabs, private state, downloads, fullscreen/Escape, zoom, bookmark search/removal and PDF export.

The actual login QML rendered with a session-model fixture; password masking, correct authentication argument routing and clearing the field on failed authentication passed. This is a UI/proxy test, not an installed PAM authentication test.

The finished ISO was mounted read-only. Build identity, default widget/icon styles, both desktop and window themes, the login theme, executable live/installed payloads and original wallpaper hashes passed inspection. Both firmware menus rendered from the final ISO. The two release parts were verified to reconstruct its SHA-256.

An isolated QEMU UEFI VM with 3 GiB RAM, no network and no hard disk reached the Plasma desktop, Aurora wallpaper, bottom taskbar and Synterra Hub. The final live image completed Light → Dark → Light switching, including panel, window frames and the open Hub. A selected Graphite wallpaper was retained during the Dark Glass switch; Aurora was then selected and remained in place when returning to Light Glass. Screenshots were visually inspected.

## Remaining validation

A complete installation to a disposable VM disk, installed login authentication, encrypted installations, BIOS desktop startup, wider hardware compatibility, physical Bluetooth/KDE Connect pairing and broad website/download-cancellation behavior still need testing. Release signing is future work.

At Archinstall completion choose Exit and wait for Synterra finalization before rebooting. If a guard fails, keep the live session running and inspect the reported condition.
