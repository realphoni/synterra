# Build status — 2026-10-02

Synterra Indev (Prism 0.3) built successfully with Archiso in `SynterraBuild`.

- Image: `synterra-0.3.20261002-x86_64.iso`.
- Size: 2,526,052,352 bytes.
- SHA-256: `f510a0ebff1f8c174888911c67fa5b56bdf5cd1e58cf9e2863a5cac77b8555c4`.
- Release: two ISO parts, whole-image checksum and part checksums.

## Prism 0.3

Synterra Surf replaces Firefox. It is a native C++/Qt WebEngine browser with tabs, bookmarks, downloads, zoom and an original local start page. Surf is included in both the live desktop and installed-system payload, with default HTTP/HTTPS associations and a taskbar launcher. Spectacle and Gwenview are included, alongside the Aero-inspired desktop, wallpapers and custom Fastfetch logo.

The installer includes the COSMIC-to-SDDM alias fix. Archinstall no longer separately enables SDDM during its base phase; Synterra finalization explicitly selects SDDM with a forced alias update. Choose Exit at Archinstall's completion screen and wait for Synterra finalization before rebooting. The guarded recovery helper is built into the live application menu.

## Validation

Rust compilation, source validation, shell parser checks and installer guard/regression tests passed. The installer regression recreated a COSMIC greeter alias and verified the SDDM replacement in a disposable directory. Browser URL/search tests passed; its local start page rendered and passed DOM checks under an unprivileged user with Chromium's sandbox enabled. The rendered browser screenshot was visually inspected.

Archiso completed package installation, initramfs, SquashFS and ISO creation. The compressed installer payload was checked for Surf's executable permissions. The supplied wallpapers remain unchanged.

Prior user screenshots confirm live VMware boots. This 0.3 ISO has not yet been boot-tested or installed to a disposable virtual disk. Broad website compatibility and interactive browser download/permission flows need manual testing. Release signing remains future work.
