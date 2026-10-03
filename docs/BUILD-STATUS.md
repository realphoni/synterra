# Build status — 2026-10-03

Synterra 1.0 (Prism) built successfully with official Archiso in `SynterraBuild`.

- Image: `synterra-1.0.20261003-x86_64.iso`.
- Size: 2,407,563,264 bytes (about 2.24 GiB).
- SHA-256: `abc162802fd5461e17be5b85addc430af924fcf74cd531ff636c5dda8ab4f6f2`.
- Release: `v1.0.0`, with two ISO parts, whole-image and part checksums, Surf 1.0 update archive/checksum and screenshots.

## Changes

OS, installer, browser, desktop and boot branding use Synterra 1.0 (Prism). Native Synterra Hub provides settings, application launchers, phone integration, installation and administrator-assisted updates. First-login welcome preferences are saved locally; updates are unavailable in live sessions. Official KDE Connect, KCalc, Bluedevil and BlueZ packages are included.

Surf 1.0 adds PDF export, fullscreen, a zoom indicator and searchable bookmarks with individual removal. Its Chromium engine comes from Arch's Qt WebEngine package. Standard Fastfetch remains available without the obsolete custom logo or preset.

Both UEFI GRUB and BIOS Syslinux display Synterra artwork. The original Aurora and Graphite wallpapers are unchanged. Installed GRUB themes stay under `/boot/grub/themes/SynterraPrism`, with managed settings that preserve existing kernel and encryption options.

The installer now uses Archinstall's current `bootloader_config` format, checks for a root fstab entry and real user-home directories before copying its payload, and enables Bluetooth during finalization. The COSMIC-to-SDDM repair and guarded recovery helper remain included.

## Validation

Rust source validation and installer target/greeter regressions passed, including incomplete fstab and symlinked-home rejection. Bash and GRUB syntax checks passed. Native Surf and Hub compilation passed. Sandboxed, unprivileged Surf tests exercised real Chromium rendering, find results, tabs, private state, an HTTP download fixture, F11/Escape, zoom, bookmark search/removal and Ctrl+P PDF output. Hub rendered with its embedded orb and passed both live/installed action checks.

The compressed final ISO was mounted read-only. Live and installed-payload versions, executable Hub/Surf/welcome/theme helpers, welcome autostart, Bluetooth service and added applications were verified. Both original wallpapers match their source SHA-256 hashes in live and installed payloads. The GRUB background also matches the original Aurora PNG. The BIOS splash is a generated 640 × 480 derivative.

Both firmware menus were rendered from the final ISO in isolated QEMU without a writable disk or network and visually inspected. A separate six-minute UEFI live boot with 3 GiB RAM reached the KDE desktop, Aurora wallpaper, Aero taskbar and Synterra Hub. Its screenshot shows the live install button and disabled update control. This confirms desktop startup; it does not prove a full installation or every application/device integration.

## Remaining validation

A complete installation to a disposable VM disk, encrypted installations, BIOS desktop startup, wider hardware compatibility, physical Bluetooth/KDE Connect pairing and broad website/download-cancellation behavior still need testing. Release signing is future work. The 1.0 name does not remove these limitations.

At Archinstall completion choose Exit and wait for Synterra finalization before rebooting. If an installer guard fails, keep the live session running and inspect the reported condition rather than rebooting an incomplete target.