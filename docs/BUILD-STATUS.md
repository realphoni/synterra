# Build status — 2026-10-02

Synterra Indev (Prism 0.5) built successfully with Archiso in `SynterraBuild`.

- Image: `synterra-0.5.20261002-x86_64.iso`.
- Size: 2,398,257,152 bytes.
- SHA-256: `11d6e6d684d696f7006beb768ed0cefecc688832eddd7b4ab62acb0204e6cf52`.
- Release assets: two ISO parts, whole-image and part checksums, and a screenshot of the finished ISO's UEFI GRUB screen.

## Prism 0.5

The custom Fastfetch logo and preset are removed; standard Fastfetch remains installed. UEFI live boot uses GRUB with the original Aurora wallpaper, Synterra orb, glass selection and countdown. Legacy BIOS live boot retains Archiso's Syslinux menu. GRUB is the installer's default bootloader; finalization installs the same theme on configured GRUB systems. Other bootloaders are retained without a GRUB splash.

Theme resources are copied into `/boot/grub/themes/SynterraPrism` to support a separate boot partition and encrypted root. Managed settings and a font loader preserve existing `/etc/default/grub` kernel/encryption options. Theme generation uses official GRUB fonts, DejaVu and librsvg. Latin font ranges and an explicit terminal font avoid oversized Unicode glyph metrics in GRUB.

Surf and the desktop applications from Prism 0.4 remain included. Original wallpapers and desktop contrast are unchanged.

## Validation

Rust source validation and installer target/greeter regression checks passed. Bash and GRUB syntax checks passed. Native Surf compilation and self-tests passed during profile generation. Theme staging in a disposable target preserved the existing kernel/encryption settings.

The finished ISO was inspected through a read-only mount. Its GRUB loader and theme files are present; obsolete Fastfetch branding is absent from SquashFS. Live and installer-payload theme helpers are executable. Desktop contrast and the original Aurora wallpaper were verified in the compressed image; the GRUB background has the same SHA-256 as the supplied original.

Both firmware boot menus were rendered from the final ISO in isolated QEMU with no writable disk or network. Screenshots were visually inspected: UEFI displays the Synterra splash and countdown; BIOS displays the supported Syslinux fallback. This verifies the boot menus, not the full desktop session or an installation. A full installation to a disposable VM disk, encrypted installations and broader hardware compatibility still need manual validation. Release signing remains future work.

At Archinstall completion choose Exit and wait for Synterra finalization before rebooting. The installer retains the COSMIC-to-SDDM repair and guarded recovery entry. The online recovery helper retains a plain GRUB configuration fallback for older live-image payloads that do not contain the new theme helper.
