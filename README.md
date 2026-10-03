# Synterra 1.0 (Prism)

An Arch-based x86_64 live distribution with an original Aero-inspired KDE Plasma desktop.

## Download the live ISO

Get the two ISO parts, `SHA256SUMS` and `SHA256SUMS.parts` from the [Synterra 1.0 (Prism) release](https://github.com/realphoni/synterra/releases/tag/v1.0.0). The image is 2,407,563,264 bytes (about 2.24 GiB), exceeding GitHub's 2 GiB limit per release asset, so it is distributed in two parts.

On Linux, put all four files in one folder, then verify and join them:

```bash
sha256sum -c SHA256SUMS.parts
cat synterra-1.0.20261003-x86_64.iso.part00 synterra-1.0.20261003-x86_64.iso.part01 > synterra-1.0.20261003-x86_64.iso
sha256sum -c SHA256SUMS
```

On Windows, use Command Prompt to join the parts:

```cmd
copy /b synterra-1.0.20261003-x86_64.iso.part00+synterra-1.0.20261003-x86_64.iso.part01 synterra-1.0.20261003-x86_64.iso
```

Then check the assembled ISO in PowerShell:

```powershell
Get-FileHash .\synterra-1.0.20261003-x86_64.iso -Algorithm SHA256
```

Expected SHA-256: `abc162802fd5461e17be5b85addc430af924fcf74cd531ff636c5dda8ab4f6f2`.

Attach the assembled `.iso` to a VMware test VM's virtual CD/DVD drive. The parts themselves are not bootable. This is an experimental live image; see the testing and installation limitations below.

## New in Synterra 1.0

- **Synterra Hub** brings display, network, Bluetooth, phone integration, applications and update controls into one native desktop welcome window. It appears once on first sign-in; choose “Show this welcome when I sign in” to keep it. Open it anytime from the application menu.
- **KDE Connect** pairs your phone with the desktop, **KCalc** adds a calculator, and **Bluedevil/BlueZ** provide Bluetooth controls. Pair devices explicitly through their setup screens.
- **Surf 1.0** adds PDF export (Ctrl+P), full-screen mode (F11, Escape to exit), a zoom indicator and searchable bookmarks with individual removal. Private-window PDF exports remain on disk like downloads.
- UEFI GRUB and BIOS Syslinux now share Synterra artwork using your original Aurora wallpaper and orb. The custom fetch logo remains removed.
- The installer uses Archinstall's current GRUB configuration format and rejects incomplete root fstab entries or unsafe user-home paths before copying desktop files.

### Synterra Hub

Use Get started for desktop settings and phone pairing, Applications for the included apps, and About & help for system information and documentation. System updates open the normal administrator-assisted `pacman -Syu` transaction in a terminal so you can review changes. Updates are disabled in the live session; they become available after installation. Bluetooth and KDE Connect still need testing with physical devices.

## New in Prism 0.5

- **Synterra GRUB splash:** original Aurora wallpaper, Synterra orb, glass selection and a visible boot countdown.
- UEFI live images now boot through GRUB. Legacy BIOS live images retain Archiso's Syslinux menu.
- The installer selects GRUB by default and configures the same splash on installed systems. Its managed theme settings preserve existing kernel and encryption options.
- The custom Fastfetch logo and preset have been removed. Standard `fastfetch` remains available.

## Included from Prism 0.4

- **Synterra Surf** gains private windows, searchable browsing history, find-in-page, reopen-closed-tab, optional session restoration, favicons and a dark-toolbar setting.
- Downloads now have a progress window with cancellation and folder access. Closing a window with active downloads asks before cancelling them.
- Improved address parsing supports ports with paths and IPv6 addresses. Saved history and sessions strip URL credentials; history keeps up to 200 recent pages.
- **Okular** opens PDFs, **VLC** plays media and **Filelight** visualizes disk usage. Spectacle and Gwenview remain included.
- Aero-inspired desktop, original wallpapers and installer recovery continue from previous releases.

## Synterra Surf

Surf is a native C++/Qt application using Arch's `qt6-webengine` package for its Chromium rendering engine. Its source is in `browser/`; CMake and Ninja compile it into both the live image and installed-system payload. Arch updates supply the engine.

Use the Menu button for history, settings and private windows. Session restoration is off by default; enable it in Settings to save up to 30 open web tabs when the window closes. History is saved locally and can be searched or cleared from its dialog. Bookmarks from earlier Surf versions are retained.

| Shortcut | Action |
| --- | --- |
| Ctrl+L / Ctrl+T / Ctrl+W | Address bar / new tab / close tab |
| Ctrl+Shift+T / Ctrl+Shift+N | Reopen closed tab / private window |
| Ctrl+F / Escape | Find in page / close find bar or stop loading |
| Ctrl+H / Ctrl+J / Ctrl+D | History / downloads / bookmarks |
| Ctrl+R / Alt+Left / Alt+Right | Reload / back / forward |
| Ctrl+P / F11 | Save page as PDF / toggle full screen |
| Ctrl++ / Ctrl+- / Ctrl+0 | Zoom in / zoom out / reset zoom |

Private windows use a separate [Qt off-the-record profile](https://doc.qt.io/qt-6/qwebengineprofile.html#QWebEngineProfile), with memory-only cookies/cache and no Surf history or session persistence. Downloads and explicitly saved bookmarks remain on disk. Private browsing does not hide traffic from websites or network operators. Website permissions still ask, and normal certificate validation and Chromium sandboxing stay enabled.

Surf remains an early browser: extensions, a password manager and Firefox profile migration are not implemented. Tests cover address parsing, history limits/deduplication, credential stripping, session/private isolation, actual page rendering, Ctrl+F/find results, tab closure, an HTTP download fixture, F11/Escape, zoom, bookmark search/removal and Ctrl+P PDF export. Broad website compatibility, download cancellation and full desktop integration still need manual testing.

### Update Surf on an existing Synterra installation

Download `synterra-surf-1.0.0-linux-x86_64.tar.gz` and `SHA256SUMS.surf` from the [Synterra 1.0 release](https://github.com/realphoni/synterra/releases/tag/v1.0.0). Put both downloads in one folder and close Surf, then run:

```bash
sha256sum -c SHA256SUMS.surf
tar -xzf synterra-surf-1.0.0-linux-x86_64.tar.gz
cd synterra-surf-1.0.0-linux-x86_64
sudo pacman -Syu qt6-webengine qt6-base
sudo install -Dm755 synterra-surf /usr/local/bin/synterra-surf
sudo install -Dm644 synterra-surf.desktop /usr/share/applications/synterra-surf.desktop
sudo install -Dm644 synterra-surf.svg /usr/share/icons/hicolor/scalable/apps/synterra-surf.svg
```

The archive targets current x86_64 Arch/Synterra systems. It requires Qt libraries and is not a standalone Windows executable. Existing profiles and bookmarks are retained. Build the archive locally with `bash scripts/package-surf.sh` after compiling Surf.

## Desktop

- Aurora wallpaper by default; Graphite available in the wallpaper picker. Both original 3840 × 2160 PNGs are included unchanged.
- Synterra Glass Plasma style: translucent blue glass, highlight edges, blur masks and Breeze fallback for controls.
- Synterra Glass Aurorae window frames: smooth translucent side borders, pale title text, glossy buttons and a red close button.
- Bottom taskbar with launcher, pinned applications, tray, clock and Show Desktop.
- Light application surfaces, blue selection colors, Noto Sans and Breeze icons. Wayland session, PipeWire audio, NetworkManager and VMware guest tools.

## Build inside the Manjaro VM

Allocate at least 4 vCPUs, 8 GB RAM and 35 GB of free space. Copy this folder into the VM (for example `~/synterra`); build on the guest's Linux filesystem, not directly on a VMware shared folder.

## Build with WSL on this Windows workspace

The first ISO was built using WSL 3.0.1 and a dedicated official Arch Linux distribution named `SynterraBuild`. The official Arch WSL image was downloaded and SHA-256 verified under `out/installers/`.

On the configured Windows workspace, run `scripts/resume-wsl-build.ps1` from PowerShell. It registers `SynterraBuild` if needed (using the downloaded image), installs Arch's build dependencies, validates Synterra's source with Rust, runs Archiso on the distribution's Linux filesystem, and copies the ISO, checksums and logs to `E:\tuff\out`. New WSL installations may require a Windows restart before registration works. The helper assumes this workspace is at `E:\tuff`; native Arch and Manjaro instructions below are portable.

```powershell
& E:\tuff\scripts\resume-wsl-build.ps1
```

### Manjaro build instructions

The public GitHub repository is the preferred transfer method. Clone over HTTPS; authentication is not required:

```bash
git clone https://github.com/realphoni/synterra.git ~/synterra
cd ~/synterra
sudo bash scripts/build-manjaro.sh
```

If an earlier failed copy created `~/synterra`, clone into `~/synterra-git` instead and build from that directory. To update an existing clone, run `git pull --ff-only` there before building.

Host prerequisites: `bash`, `curl`, `gnupg`, `tar`, `zstd`, `util-linux`, and `/usr/share/pacman/keyrings/archlinux.gpg`. Install missing prerequisites using Manjaro's package manager. Do not replace Manjaro's repositories with Arch repositories.

The existing Manjaro VM has a read-only VMware share named `Synterra` pointing to `E:\tuff`. If it is mounted at `/mnt/hgfs/Synterra`, copy it into the guest with `cp -a /mnt/hgfs/Synterra ~/synterra`. If the share is not mounted, open it through VMware shared folders or mount it with `sudo mkdir -p /mnt/hgfs` then `sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other`.

```bash
cd ~/synterra
sudo bash scripts/build-manjaro.sh
```

The script verifies the official Arch bootstrap signature against the installed Arch keyring, creates an isolated Arch chroot in `/var/tmp`, installs Arch's build tools there, generates a profile from that version's `releng` template, and builds the live image. Host Manjaro repositories and installed desktop are untouched. Logs and ISO checksums are written under `out/`. The build root is retained for inspection. It can contain tens of GB; unmount it before removing it manually.

On a native Arch build host with `archiso`, `rust`, `gcc`, `cmake`, `ninja`, `pkgconf`, `qt6-webengine`, `qt6-svg`, `grub`, `librsvg` and `ttf-dejavu` installed:

```bash
sudo bash scripts/build-arch.sh
```

`ARCHISO_PROFILE` may select a local releng template. Each build uses a fresh profile and work directory to avoid stale archiso artifacts. Arch packages are rolling releases, so builds are not bit-for-bit reproducible without pinned package snapshots.

## Test in VMware

Boot the generated ISO in a separate test VM (Other Linux 6.x 64-bit, UEFI, 3D acceleration enabled). Keep Manjaro as the builder. Verify:

1. The live session enters Plasma automatically as `live`.
2. Aurora is visible, the panel is at the bottom, and glass frames have working minimize/maximize/close buttons.
3. Network, sound, resize, clipboard and the wallpaper picker work.
4. `cat /etc/os-release` identifies Synterra; `pacman -Si plasma-desktop` uses Arch repositories.
5. Verify the Synterra GRUB splash on UEFI. Reboot and test the Syslinux fallback on BIOS.

The live account has no password; root administration is available through sudo. SSH is not enabled. The Rust installer wraps Archinstall and applies Synterra's desktop to the installed system. It uses a terminal wizard rather than a graphical partition editor. Release signing remains future work.

## Install Synterra

Open **Install Synterra** from the desktop or application menu. The Rust launcher starts Archinstall's guided terminal interface with KDE Plasma, Synterra's packages and NetworkManager selected. Connect to the internet first; installation downloads packages from Arch mirrors.

Choose the target disk, partition layout, timezone, bootloader and a regular administrator account with a password. GRUB is selected by default; retain it for the Synterra splash. Other bootloaders do not display the GRUB theme. Review the disk summary before confirming: formatting erases existing data. Keep the KDE profile and Synterra package list selected.

**At Archinstall's completion screen choose Exit, not Reboot.** The wrapper then copies Synterra's desktop and GRUB themes, wallpapers and user defaults, applies branding and enables the desktop services. Wait for **Synterra installation finished**, then shut down, eject the live ISO and boot from the installed disk.

Installed systems use the account you created. Live-session autologin, the live account and passwordless sudo are not copied. The installer rejects cancelled or incomplete base installations and targets without a regular account. This installer is experimental; a full installation on a disposable VM disk still needs validation.

## GRUB splash

The splash uses your original Aurora wallpaper unchanged. Arrow keys select an entry; Enter boots, E edits and C opens GRUB's console. Theme sources live in `assets/grub/`; the builder generates fonts and glass frame PNGs using GRUB and librsvg.

Installed GRUB systems keep the theme under `/boot/grub/themes/SynterraPrism`, including when `/boot` is separate from an encrypted root. Settings live in `/etc/default/grub.d/10-synterra.cfg`; `/etc/default/grub` retains its existing disk and kernel options. To refresh the theme on an installed Synterra GRUB system, run `sudo /usr/local/libexec/synterra-grub-setup`.

## Recover a Prism 0.2 greeter conflict

The published 0.2 ISO can stop with `display-manager.service already exists ... cosmic-greeter.service` if Archinstall selected COSMIC's greeter. The installer source is corrected: the base stage leaves greeter selection to Archinstall, then Synterra finalization explicitly selects SDDM. The existing ISO assets have not been rebuilt with this fix.

For an installation still mounted at `/mnt/synterra-install`, run this from a new terminal in the same live session:

```bash
curl -fL https://raw.githubusercontent.com/realphoni/synterra/main/scripts/recover-install.sh -o /tmp/synterra-recover.sh
sudo bash /tmp/synterra-recover.sh
```

The helper checks the mounted root, installed packages and (on UEFI) the mounted EFI partition and bootloader. It asks you to type `REPAIR`, then restores Synterra's desktop, switches the login service to SDDM and generates fstab with a backup. If no regular user was created, it prompts for a new administrator account and password inside the VM. It does not partition or format disks. Wait for `Synterra recovery finished` before shutting down and removing the ISO. If it stops at a guard check, retain the live session and inspect the reported condition.

## Files

- scripts/: verified-bootstrap builder, Arch builder, profile generator and theme validator.
- desktop/: native Qt Synterra Hub.
- `profile/packages.txt`: extra packages layered onto upstream Arch releng.
- `profile/airootfs/`: branding, live-session configuration and desktop defaults.
- `assets/wallpapers/`: supplied originals, copied to the image during profile generation.
- `assets/grub/`: GRUB layout, live loader and BIOS splash artwork.
- `docs/BUILD-STATUS.md`: actual validation and VMware status.

## Rust development tools

The project's artwork generator, source validator and bootstrap signature regression are Rust utilities in `src/`. Generated desktop assets are committed, so the ISO builder can use them directly. Archiso's build and mount orchestration remains Bash. The Synterra package manifest includes Rust and its Cargo provider; upstream desktop packages may still use their own language runtimes.

In Manjaro, install Rust using the official rustup installer as your normal user:

```bash
bash scripts/install-rust.sh
source "$HOME/.cargo/env"
```

A native linker is also required: install Manjaro's `base-devel` package group if it is missing. Run the utilities with the committed dependency lockfile:

```bash
cargo run --locked -- generate-artwork
cargo run --locked -- validate
cargo run --locked -- test-bootstrap-signature
cargo run --locked -- test-installer
```

The signature regression requires GPG and uses throwaway keys. Windows builds use Rust with Visual Studio's C++ tools; run Cargo from a Developer PowerShell or Developer Command Prompt so Microsoft's linker is selected.

## Upstream and licensing

Uses [Archiso](https://wiki.archlinux.org/title/Archiso), [KDE Plasma styles](https://develop.kde.org/docs/plasma/theme/) and [Aurorae](https://develop.kde.org/docs/plasma/aurorae/). Synterra is an independent derivative; it is not an official Arch, Manjaro or Microsoft product. Original code and vector artwork in this repository are MIT licensed. Wallpaper ownership remains with their creator; their inclusion here does not grant redistribution rights. Upstream packages retain their own licenses.

