# Synterra Indev (Prism 0.3)

An Arch-based x86_64 live distribution with an original Aero-inspired KDE Plasma desktop.

## Download the live ISO

Get the two ISO parts, `SHA256SUMS` and `SHA256SUMS.parts` from the [Synterra Indev Prism 0.3 release](https://github.com/realphoni/synterra/releases/tag/v0.3.20261002-alpha). The image is 2,526,052,352 bytes (about 2.3 GiB), exceeding GitHub's 2 GiB limit per release asset, so it is distributed in two parts.

On Linux, put all four files in one folder, then verify and join them:

```bash
sha256sum -c SHA256SUMS.parts
cat synterra-0.3.20261002-x86_64.iso.part00 synterra-0.3.20261002-x86_64.iso.part01 > synterra-0.3.20261002-x86_64.iso
sha256sum -c SHA256SUMS
```

On Windows, use Command Prompt to join the parts:

```cmd
copy /b synterra-0.3.20261002-x86_64.iso.part00+synterra-0.3.20261002-x86_64.iso.part01 synterra-0.3.20261002-x86_64.iso
```

Then check the assembled ISO in PowerShell:

```powershell
Get-FileHash .\synterra-0.3.20261002-x86_64.iso -Algorithm SHA256
```

Expected SHA-256: `f510a0ebff1f8c174888911c67fa5b56bdf5cd1e58cf9e2863a5cac77b8555c4`.

Attach the assembled `.iso` to a VMware test VM's virtual CD/DVD drive. The parts themselves are not bootable. This is an experimental live image; see the testing and installation limitations below.

## New in Prism 0.3

- **Synterra Surf** replaces Firefox as the default web browser and taskbar launcher. It has tabs, address/search navigation, bookmarks, downloads, zoom shortcuts and an original Synterra start page.
- **Spectacle** captures screenshots and **Gwenview** opens images.
- The installer includes the tested COSMIC-to-SDDM greeter fix. **Repair Synterra Installation** is also available in the live application menu, so recovery no longer requires downloading a helper.
- The smooth glass borders, original wallpapers and custom Fastfetch logo continue from Prism 0.2.

## Synterra Surf

Surf is a native C++/Qt application using Arch's `qt6-webengine` package for its Chromium-based rendering engine. Its source is in `browser/`; it is compiled with CMake and Ninja during ISO preparation and included in both the live image and installed-system payload. Upstream engine updates come from Arch's packages.

Use Ctrl+L for the address bar, Ctrl+T for a new tab, Ctrl+W to close a tab, Ctrl+R to reload, and Ctrl++ / Ctrl+- to zoom. The Bookmarks menu saves pages locally. Downloads ask where to save; the Downloads button opens the download folder. Web addresses use HTTPS by default and searches use DuckDuckGo. Website permissions ask before access is granted; certificate validation and the engine sandbox retain their defaults.

This is an early browser. Extensions, a password manager, session restoration and Firefox profile migration are not implemented. Source tests verify address normalization and search encoding; a sandboxed engine smoke test rendered and checked the local start page. Broad website compatibility and interactive download/permission flows still need manual testing.

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

On a native Arch build host with `archiso`, `rust`, `gcc`, `cmake`, `ninja`, `pkgconf` and `qt6-webengine` installed:

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
5. Reboot and test BIOS boot as well as UEFI boot.

The live account has no password; root administration is available through sudo. SSH is not enabled. The Rust installer wraps Archinstall and applies Synterra's desktop to the installed system. It uses a terminal wizard rather than a graphical partition editor. Release signing remains future work.

## Install Synterra

Open **Install Synterra Indev** from the desktop or application menu. The Rust launcher starts Archinstall's guided terminal interface with KDE Plasma, Synterra's packages and NetworkManager selected. Connect to the internet first; installation downloads packages from Arch mirrors.

Choose the target disk, partition layout, timezone, bootloader and a regular administrator account with a password. Review the disk summary before confirming: formatting erases existing data. Keep the KDE profile and Synterra package list selected.

**At Archinstall's completion screen choose Exit, not Reboot.** The wrapper then copies Synterra's theme, wallpapers, Fastfetch logo and user defaults, applies branding and enables the desktop services. Wait for **Synterra installation finished**, then shut down, eject the live ISO and boot from the installed disk.

Installed systems use the account you created. Live-session autologin, the live account and passwordless sudo are not copied. The installer rejects cancelled or incomplete base installations and targets without a regular account. This installer is experimental; a full installation on a disposable VM disk still needs validation.

## Fastfetch

Run `fastfetch` to display the custom blue/cyan/violet Synterra Prism logo. Defaults are installed through `/etc/skel/.config/fastfetch/config.jsonc`, and the logo lives at `/usr/share/synterra/fastfetch-logo.txt`.

## Recover a Prism 0.2 greeter conflict

The published 0.2 ISO can stop with `display-manager.service already exists ... cosmic-greeter.service` if Archinstall selected COSMIC's greeter. The installer source is corrected: the base stage leaves greeter selection to Archinstall, then Synterra finalization explicitly selects SDDM. The existing ISO assets have not been rebuilt with this fix.

For an installation still mounted at `/mnt/synterra-install`, run this from a new terminal in the same live session:

```bash
curl -fL https://raw.githubusercontent.com/realphoni/synterra/main/scripts/recover-install.sh -o /tmp/synterra-recover.sh
sudo bash /tmp/synterra-recover.sh
```

The helper checks the mounted root, installed packages and (on UEFI) the mounted EFI partition and bootloader. It asks you to type `REPAIR`, then restores Synterra's desktop, switches the login service to SDDM and generates fstab with a backup. If no regular user was created, it prompts for a new administrator account and password inside the VM. It does not partition or format disks. Wait for `Synterra recovery finished` before shutting down and removing the ISO. If it stops at a guard check, retain the live session and inspect the reported condition.

## Files

- `scripts/`: verified-bootstrap builder, Arch builder, profile generator and theme validator.
- `profile/packages.txt`: extra packages layered onto upstream Arch releng.
- `profile/airootfs/`: branding, live-session configuration and desktop defaults.
- `assets/wallpapers/`: supplied originals, copied to the image during profile generation.
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

