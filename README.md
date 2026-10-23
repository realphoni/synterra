# Synterra

An Arch-based x86_64 live distribution with Synaqua, the Aero-inspired Synaqua desktop using KWin/Plasma services.

Development release: **Synterra 2.0 (Variscite), Build 2020**, focused on **Synterra Surf 1.2**. Download the [Build 2020 prerelease](https://github.com/realphoni/synterra/releases/tag/v2.0.2020). The 1.1 full release remains available below.

## New in Build 2020: Surf notices

Surf 1.2 adds a notice strip below the address bar. Blue information notices and amber warnings follow the active site or supported search topic. Notices can be dismissed per tab, restored from Menu, cycled when several topics match, or disabled in Settings. There are separate controls for search topics and playful messages. Source buttons open only when clicked.

The 21 built-in topics include Roblox, OpenAI/ChatGPT, Gemini, X/Twitter, Momentum, Tactico, Synterra, Phoni Technology, Steam, AO3 and Ace Attorney. Ace Attorney also has a rare alternate message. Matching happens on the device, without scanning page content or sending notice checks to a service. Private-window dismissals remain in memory.

See the [Surf notice guide and content catalog](docs/SURF-NOTICES.md) and [current build status](docs/BUILD-STATUS.md). The source catalog is `browser/notices.json`. The release includes `synterra-surf-1.2.0-linux-x86_64.tar.gz` and `SHA256SUMS.surf` for updating only the browser on an existing x86_64 Synterra/Arch installation. Close Surf, extract the archive and follow its `README.txt`; existing profiles and bookmarks are retained.

## Download Synterra 2.0 Build 2020

Download **both** `synterra-2.0.2020-x86_64.iso.part00` and `.part01`, plus `SHA256SUMS` and `SHA256SUMS.parts`, from the [Build 2020 release](https://github.com/realphoni/synterra/releases/tag/v2.0.2020). The parts are not bootable until joined.

Linux:

```bash
sha256sum -c SHA256SUMS.parts
cat synterra-2.0.2020-x86_64.iso.part00 synterra-2.0.2020-x86_64.iso.part01 > synterra-2.0.2020-x86_64.iso
sha256sum -c SHA256SUMS
```

Windows Command Prompt:

```cmd
copy /b synterra-2.0.2020-x86_64.iso.part00+synterra-2.0.2020-x86_64.iso.part01 synterra-2.0.2020-x86_64.iso
```

Then check the image in PowerShell with `Get-FileHash .\synterra-2.0.2020-x86_64.iso -Algorithm SHA256`. Expected SHA-256: `ed07f25ee0eb69b5d56553e2ddfee0cf571d45e6d32e9fb47b5e58605ae6d7e4`. Assembled size: **2,580,217,856 bytes**.

This is a development prerelease. Native browser tests, image integrity and BIOS/UEFI menu checks passed. A fresh full desktop boot and installation cycle were not rerun for Build 2020. Use a disposable test VM first. The updater accepts full releases and does not install this prerelease automatically.

## Download Synterra 2.0 Build 2010

Download both `synterra-2.0.2010-x86_64.iso.part00` and `.part01`, plus `SHA256SUMS` and `SHA256SUMS.parts`, from the [release page](https://github.com/realphoni/synterra/releases/tag/v2.0.2010). Join and verify them on Linux:

```bash
sha256sum -c SHA256SUMS.parts
cat synterra-2.0.2010-x86_64.iso.part00 synterra-2.0.2010-x86_64.iso.part01 > synterra-2.0.2010-x86_64.iso
sha256sum -c SHA256SUMS
```

On Windows, join them in Command Prompt:

```cmd
copy /b synterra-2.0.2010-x86_64.iso.part00+synterra-2.0.2010-x86_64.iso.part01 synterra-2.0.2010-x86_64.iso
```

Then run `Get-FileHash .\synterra-2.0.2010-x86_64.iso -Algorithm SHA256` in PowerShell. Expected SHA-256: `b9edd7cb4c4a03300aa62da3712a2639ca3adb96837e6f3153a264bf4cdb3602`. Assembled size: **2,580,152,320 bytes**. The parts themselves are not bootable.

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
| Ctrl+Shift+R | Reload bypassing HTTP cache |
| Ctrl+Shift+M | Mute / unmute this tab |
| F12 / Ctrl+Shift+I | Open this tab's developer tools |
| Ctrl+P / F11 | Save page as PDF / toggle full screen |
| Ctrl++ / Ctrl+- / Ctrl+0 | Zoom in / zoom out / reset zoom |

Private windows use a separate [Qt off-the-record profile](https://doc.qt.io/qt-6/qwebengineprofile.html#QWebEngineProfile), with memory-only cookies/cache and no Surf history or session persistence. Downloads and explicitly saved bookmarks remain on disk. Private browsing does not hide traffic from websites or network operators. Website permissions still ask, and normal certificate validation and Chromium sandboxing stay enabled.

Surf remains an early browser: extensions, a password manager and Firefox profile migration are not implemented. Local Chromium tests cover async JavaScript, fetch/JSON, DOM updates, local storage, user-triggered and blocked popups, opener/close callbacks, background tabs, mute isolation, address editing, developer-tool cleanup and stopped/failed/crashed-page recovery. Earlier URL, history, session/private, download, find, fullscreen, bookmark and PDF checks are retained. Broad website compatibility, download cancellation and full desktop integration still need manual testing.

### Update Surf 1.1.1 on an earlier Synterra installation

Build 1170 already includes Surf 1.1.1. The archive below updates the browser on earlier installations; it does not install Synaqua or upgrade the OS build.

Download `synterra-surf-1.1.1-linux-x86_64.tar.gz` and `SHA256SUMS.surf` from the [Build 1130 release](https://github.com/realphoni/synterra/releases/tag/v1.1.1130). Put both downloads in one folder and close Surf, then run:

```bash
sha256sum -c SHA256SUMS.surf
tar -xzf synterra-surf-1.1.1-linux-x86_64.tar.gz
cd synterra-surf-1.1.1-linux-x86_64
sudo pacman -Syu qt6-webengine qt6-base
sudo install -Dm755 synterra-surf /usr/local/bin/synterra-surf
sudo install -Dm644 synterra-surf.desktop /usr/share/applications/synterra-surf.desktop
sudo install -Dm644 synterra-surf.svg /usr/share/icons/hicolor/scalable/apps/synterra-surf.svg
```

The archive targets current x86_64 Arch/Synterra systems. It requires Qt libraries and is not a standalone Windows executable. It updates the browser only; the full Glass desktop is included in the 1.1 ISO. Existing profiles and bookmarks are retained. Build the archive locally with `bash scripts/package-surf.sh` after compiling Surf.

## Desktop

- Variscite glass wallpaper by default; Graphite remains available in the wallpaper picker. Both original Prism 3840 × 2160 source PNGs are retained. The new Variscite image is 1672 × 941.
- Synterra Glass light/dark Plasma styles: translucent glass, highlight edges and blur masks.
- Matching Synterra Glass Aurorae window frames: smooth translucent side borders, pale title text, glossy buttons and a red close button.
- Bottom taskbar with launcher, pinned applications, tray, clock and Show Desktop.
- Synterra Glass Qt 6 application controls, light/dark palettes, blue selection colors, Noto Sans and original glass icons with upstream fallback. Wayland session, PipeWire audio, NetworkManager and VMware guest tools.

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

On a native Arch build host, install `archiso`, `fakeroot`, `libarchive`, `rust`, `gcc`, `cmake`, `ninja`, `pkgconf`, `qt6-webengine`, `qt6-svg`, `qt6-declarative`, `qt6-wayland`, `layer-shell-qt`, `kservice`, `kio`, `kglobalaccel`, `kwindowsystem`, `kirigami`, `plasma-workspace`, `plasma-desktop`, `grub`, `librsvg` and `ttf-dejavu`, then run:

```bash
sudo bash scripts/build-arch.sh
```

`ARCHISO_PROFILE` may select a local releng template. Each build uses a fresh profile and work directory to avoid stale archiso artifacts. Arch packages are rolling releases, so builds are not bit-for-bit reproducible without pinned package snapshots.

## Test in VMware

Boot the generated ISO in a separate test VM (Other Linux 6.x 64-bit, UEFI, 3D acceleration enabled). Keep Manjaro as the builder. Verify:

1. The live session enters Synaqua automatically as `live`.
2. Variscite and the desktop shortcuts are visible, the floating dock has a centered Start button, and glass frames have working minimize/maximize/close buttons.
3. Test Start search, pinning, running-window controls, network/sound settings, resize, clipboard and the wallpaper picker.
4. `cat /etc/os-release` identifies Synterra; `pacman -Si plasma-desktop` uses Arch repositories.
5. Verify the Synterra GRUB splash on UEFI. Reboot and test the Syslinux fallback on BIOS.
6. Switch Light Glass → Dark Glass → Light Glass in Hub. Verify panels, frames and application colors change while your wallpaper and layout stay intact.

The live account has no password; root administration is available through sudo. SSH is not enabled. The Rust installer wraps Archinstall and applies Synterra's desktop to the installed system. It uses a terminal wizard rather than a graphical partition editor. Release signing remains future work.

## Install Synterra

Open **Install Synterra** from the desktop or application menu. The Rust launcher starts Archinstall's guided terminal interface with KDE Plasma, Synterra's packages and NetworkManager selected. Connect to the internet first; installation downloads packages from Arch mirrors.

Choose the target disk, partition layout, timezone, bootloader and a regular administrator account with a password. GRUB is selected by default; retain it for the Synterra splash. Other bootloaders do not display the GRUB theme. Review the disk summary before confirming: formatting erases existing data. Keep the KDE profile and Synterra package list selected.

**At Archinstall's completion screen choose Exit, not Reboot.** The wrapper then copies Synterra's desktop and GRUB themes, wallpapers and user defaults, applies branding and enables the desktop services. Wait for **Synterra installation finished**, then shut down, eject the live ISO and boot from the installed disk.

Installed systems select Synaqua by default, with Plasma (Wayland) available in the sign-in session selector. They use the account you created. Live-session autologin, the live account and passwordless sudo are not copied. The installer rejects cancelled or incomplete base installations and targets without a regular account. See the build validation notes for installation coverage.

## GRUB splash

The splash uses the Variscite wallpaper. Arrow keys select an entry; Enter boots, E edits and C opens GRUB's console. Theme sources live in `assets/grub/`; the builder generates fonts and glass frame PNGs using GRUB and librsvg.

Installed GRUB systems keep the theme under `/boot/grub/themes/SynterraPrism`, including when `/boot` is separate from an encrypted root. Settings live in `/etc/default/grub.d/10-synterra.cfg`; `/etc/default/grub` retains its existing disk and kernel options. To refresh the theme on an installed Synterra GRUB system, run `sudo /usr/local/libexec/synterra-grub-setup`.

## Finish a partial installation

The recovery helper can finish a mounted partial installation. Build 1170 opens it when the installation target is already mounted, without running the partitioner again. It asks for `REPAIR` before making changes.

The older published 0.2 ISO can stop with `display-manager.service already exists ... cosmic-greeter.service` if Archinstall selected COSMIC's greeter. The installer source is corrected: the base stage leaves greeter selection to Archinstall, then Synterra finalization explicitly selects SDDM. The existing ISO assets have not been rebuilt with this fix.

For an installation still mounted at `/mnt/synterra-install`, run this from a new terminal in the same live session:

```bash
curl -fL https://raw.githubusercontent.com/realphoni/synterra/main/scripts/recover-install.sh -o /tmp/synterra-recover.sh
sudo bash /tmp/synterra-recover.sh
```

The helper checks the mounted root, installed packages and (on UEFI) the mounted EFI partition and bootloader. It asks you to type `REPAIR`, then restores Synterra's desktop, switches the login service to SDDM and generates fstab with a backup. If no regular user was created, it prompts for a new administrator account and password inside the VM. It does not partition or format disks. Wait for `Synterra recovery finished` before shutting down and removing the ISO. If it stops at a guard check, retain the live session and inspect the reported condition.

## Files

- scripts/: verified-bootstrap builder, Arch builder, profile generator and theme validator.
- desktop/: native Qt Synterra Hub, Glass widget plugin and preview checks.
- `profile/packages.txt`: extra packages layered onto upstream Arch releng.
- `profile/airootfs/`: branding, live-session configuration and desktop defaults.
- `assets/wallpapers/`: the Variscite default, its generation prompt and supplied Prism originals.
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

