# Build status — 2026-10-02

Synterra Indev (Prism 0.2) was built successfully with Archiso in the Arch WSL distribution `SynterraBuild`. The filename uses the UTC build date.

- Image: `synterra-0.2.20261001-x86_64.iso`.
- Size: 2,462,842,880 bytes.
- SHA-256: `bd1496231ccc93a6d63df3c3452eb28930928d1ad22c125dfa26a93eadf94721`.
- Release: two ISO parts, plus whole-image and part checksums.

## Prism 0.2 changes

Window sides and bottom edges now use a uniform translucent tint rather than repeating the titlebar gradient. Side and bottom border widths are reduced from seven to four pixels. The glossy titlebar is retained.

Fastfetch includes a custom blue/cyan/violet Synterra Prism logo and per-user defaults. Fastfetch was executed in the built filesystem and displayed the logo and `Synterra Indev (Prism 0.2)` successfully.

A Rust installer is available from the desktop, application menu and welcome dialog. It opens Archinstall's guided interface with KDE Plasma and Synterra packages selected. On successful completion it applies an installation payload containing the theme, wallpapers, Fastfetch settings and user defaults. The installed payload excludes the live account, live autologin, welcome autostart and passwordless sudo. Users must choose Exit at the Archinstall completion screen so Synterra can finalize before rebooting.

## Validation

Rust compilation, source validation, shell parser checks and installer guard tests passed. Guard tests reject incomplete targets, live-only accounts and unsafe user-home paths. Installer arguments and NetworkManager presets were checked against the Archinstall version included in the image. Archiso completed package installation, initramfs generation, SquashFS compression and ISO creation. The supplied wallpapers remain unchanged.

The user supplied a screenshot confirming the previous 0.1 live desktop booted in VMware. Prism 0.2's revised visuals and a full installation to a disposable virtual disk have not yet been tested. This is an experimental development image, and release signing remains future work.
