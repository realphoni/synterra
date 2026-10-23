# Build status — 2026-10-22

Synterra 2.0 (Variscite), Build 2010 is a **development prerelease**, tagged [v2.0.2010](https://github.com/realphoni/synterra/releases/tag/v2.0.2010). The existing public 1.1 download is unchanged; this build is published for testing rather than marked Latest.

- Image: `synterra-2.0.2010-x86_64.iso`
- Size: 2,580,152,320 bytes
- SHA-256: `b9edd7cb4c4a03300aa62da3712a2639ca3adb96837e6f3153a264bf4cdb3602`
- Desktop package: `synterra-release-2.0.2010-1-x86_64.pkg.tar.zst`
- Package size: 6,708,381 bytes
- Package SHA-256: `ab6e47f61bb1a85cee527ea7abe1c24f6f4d0d473482ca5d1c66efe22bdfc8e6`
- Update manifest and checksums: `synterra-update.json`, `SHA256SUMS.update`

The image reuses the verified 1.1 Arch package/kernel base and replaces Synterra's applications, installed desktop payload, branding and boot artwork. It is not a fresh update of all Arch packages. The normal full Archiso build remains available through the repository's build scripts.

## Setup correction

Inspection of the released 1.1.0 image reproduced the reported defect: `/etc/sudoers.d` is root-owned but mode `0777`; sudo rejects it and ignores the live user's rule. The image's `/` and `/etc` also inherited `0777` from Windows copies. The administrator account chosen in Archinstall is unrelated to this live-session failure.

Build 2010 explicitly sets root ownership and mode `0755` on these directories and `0440` on the policy files. Overlay normalization prevents synthetic DrvFS modes from reaching future builds. Live setup checks policy syntax before the display manager starts. Real elevation is tested separately to avoid opening a PAM user session before SDDM. The installed desktop payload does not include live accounts or passwordless sudo.

The real sudo regression passed against the candidate's extracted filesystem and again on the final staged root: valid syntax, successful elevation as `live`, rejection after reproducing mode `0777`, successful startup repair, and no grant to an unrelated account. The final ISO's helper hash and policy modes match the tested input. The regression runs in a disposable mount namespace and overlay. A text-console root login using the live image's PAM configuration also succeeded in isolation, verifying the [recovery instructions for the existing 1.1 ISO](LIVE-SUDO-RECOVERY.md).

The candidate also booted in a disposable UEFI VM. Root console login succeeded,
`runuser -u live -- sudo -n id` returned UID 0, all three repaired directories
were root-owned `0755`, and `synterra-live-setup.service` was active. This verifies
the reported sudo failure is fixed in the running candidate.

The actual installed Archiso overlay-copy function also passed a fixture check:
root/policy modes, ordinary data remaining `0644`, and executable live/installed
helpers. Root is overridden with `/.` because Archiso interprets a trailing slash
as recursive. Explicit overrides also guarantee executable permissions for the
installed Synaqua session wrappers.

## Variscite and Synterra Updates

Synterra Updates checks the official GitHub Releases API, downloads a newer compatible full release and verifies its size and SHA-256 before offering Update now or Later. The administrator helper rechecks the release and package and keeps pacman's package-review prompts. No installation or reboot happens automatically. See the [release update protocol and limits](UPDATES.md).

The new Variscite default wallpaper uses jade, teal and blue glass ribbons inspired by Prism. The original generated asset is 1672×941; it is not a native 4K image. The desktop, sign-in theme and GRUB share it. The legacy wallpaper URI is retained for compatibility, and Graphite remains available. The source artwork and generation prompt are in `assets/wallpapers/`.

## Validation

Source validation, installer regressions, Surf self-tests, native compilation and updater tests passed. Updater tests exercise numeric versions, full-release policy, manifest/asset mismatch, unsafe package contents, actual HTTP streaming, cache reuse/repair, oversized/corrupt downloads, cancellation and refused redirects.

An actual pacman installation into an isolated root verified migration of an unowned Synterra file, package ownership, retained dock settings and Surf bookmark fixtures, and exclusion of live accounts/autologin. This is not a complete installed-system release upgrade. The native updater rendered in the live image's runtime and previously opened in the running candidate, querying the real public GitHub feed successfully.

The final ISO's filesystem checksum, live/installed updater binary hashes, Unix permissions, branding and exact wallpaper bytes passed inspection. BIOS and UEFI menus booted from the final image and were visually inspected with Variscite art.

The final image then booted through UEFI with its default configuration in a
disposable standard-VGA QEMU VM. The new Variscite wallpaper, folder-view desktop
shortcuts and Synaqua dock rendered correctly. The capture is saved locally as
`out/2010-work/live-latest-2010.png`. No renderer override was applied to this boot.
The VM closed automatically after capture. Large generated filesystem copies
were removed, retaining source, compiled caches and release artifacts.

## Remaining validation

A full partitioning/install/first-login cycle and end-to-end authenticated release upgrade remain outstanding, along with encrypted installations, VMware 3D, multiple displays and broader hardware testing. The dock remains primary-display only and does not yet provide a complete tray/notification center.

The published release's earlier validation is archived in [1.1 status](BUILD-STATUS-1.1.md).
