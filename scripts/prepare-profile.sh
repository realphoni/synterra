#!/usr/bin/env bash
set -Eeuo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
dest=${1:?Usage: prepare-profile.sh NEW_OUTPUT_DIRECTORY}
upstream=${ARCHISO_PROFILE:-/usr/share/archiso/configs/releng}
[[ -f $upstream/profiledef.sh ]] || { echo 'Archiso releng profile missing.' >&2; exit 1; }
[[ ! -e $dest ]] || { echo 'Profile destination must not already exist.' >&2; exit 1; }
mkdir -p "$dest"
cp -a "$upstream/." "$dest/"
cp -a "$repo/profile/airootfs/." "$dest/airootfs/"
rm -f "$dest/airootfs/etc/skel/.config/fastfetch/config.jsonc" \
    "$dest/airootfs/usr/share/synterra/fastfetch-logo.txt"
command -v cargo >/dev/null || { echo 'Install Rust and Cargo on the Arch build host.' >&2; exit 1; }
cargo build --manifest-path "$repo/Cargo.toml" --locked --release
install -Dm755 "$repo/target/release/synterra-tools" "$dest/airootfs/usr/local/libexec/synterra-tools"
cmake -S "$repo/browser" -B "$repo/target/surf" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo/target/surf" --parallel 2
"$repo/target/surf/synterra-surf" --self-test
install -Dm755 "$repo/target/surf/synterra-surf" "$dest/airootfs/usr/local/bin/synterra-surf"
cmake -S "$repo/desktop" -B "$repo/target/hub" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo/target/hub" --parallel 2
install -Dm755 "$repo/target/hub/synterra-hub" "$dest/airootfs/usr/local/bin/synterra-hub"
install -Dm755 "$repo/target/hub/styles/libSynterraGlass.so" "$dest/airootfs/usr/lib/qt6/plugins/styles/libSynterraGlass.so"
install -Dm755 "$repo/scripts/recover-install.sh" "$dest/airootfs/usr/local/libexec/synterra-recover"
cmake -S "$repo/synaqua" -B "$repo/target/synaqua" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo/target/synaqua" --parallel 2
install -Dm755 "$repo/target/synaqua/synaqua-shell" "$dest/airootfs/usr/local/bin/synaqua-shell"
install -Dm755 "$repo/target/synaqua/kcm_synaqua_dock.so" "$dest/airootfs/usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_synaqua_dock.so"
# Keep upstream desktop views, lock-screen and logout components with their
# original licenses; Synaqua's dock/launcher and session layout are separate.
shell="$dest/airootfs/usr/share/plasma/shells/org.synterra.synaqua"
mkdir -p "$shell"
cp -a /usr/share/plasma/shells/org.kde.plasma.desktop/. "$shell/"
install -m644 "$repo/synaqua/shell-metadata.json" "$shell/metadata.json"
for variant in org.synterra.glass.desktop org.synterra.glass.dark.desktop; do
    install -m644 "$repo/synaqua/desktop-layout.js" "$dest/airootfs/usr/share/plasma/look-and-feel/$variant/contents/layouts/org.synterra.synaqua-layout.js"
done
theme="$dest/airootfs/usr/share/grub/themes/SynterraPrism"
bash "$repo/scripts/build-grub-theme.sh" "$theme"
install -m644 "$theme/bios-splash.png" "$dest/syslinux/splash.png"
mkdir -p "$dest/grub/themes"
cp -R "$theme" "$dest/grub/themes/"
install -Dm644 "$repo/assets/grub/live-theme.cfg" "$dest/grub/synterra-theme.cfg"
# Load our graphics after upstream sets up its menu, without altering kernel paths.
for config in "$dest/grub/grub.cfg" "$dest/grub/loopback.cfg"; do
    cat >> "$config" <<'GRUB'

source "${config_directory}/synterra-theme.cfg"
GRUB
done
# Installer payload contains desktop defaults, not live accounts/autologin.
payload="$dest/airootfs/usr/share/synterra/install-overlay"
mkdir -p "$payload/etc/skel" "$payload/usr/share"
cp -a "$repo/profile/airootfs/etc/skel/." "$payload/etc/skel/"
rm -f "$payload/etc/skel/Desktop/synterra-install.desktop"
for item in plasma aurorae color-schemes icons sddm; do
    cp -a "$repo/profile/airootfs/usr/share/$item" "$payload/usr/share/"
done
install -Dm644 "$repo/profile/airootfs/usr/share/synterra/os-release" "$payload/usr/share/synterra/os-release"
mkdir -p "$payload/usr/share/grub/themes"
cp -R "$theme" "$payload/usr/share/grub/themes/"
install -Dm755 "$repo/profile/airootfs/usr/local/libexec/synterra-grub-setup" "$payload/usr/local/libexec/synterra-grub-setup"
install -Dm644 "$repo/profile/airootfs/usr/share/libalpm/hooks/90-synterra-branding.hook" "$payload/usr/share/libalpm/hooks/90-synterra-branding.hook"
install -Dm755 "$repo/target/surf/synterra-surf" "$payload/usr/local/bin/synterra-surf"
install -Dm644 "$repo/profile/airootfs/usr/share/applications/synterra-surf.desktop" "$payload/usr/share/applications/synterra-surf.desktop"
install -Dm755 "$repo/target/hub/synterra-hub" "$payload/usr/local/bin/synterra-hub"
install -Dm755 "$repo/profile/airootfs/usr/local/bin/synterra-welcome" "$payload/usr/local/bin/synterra-welcome"
install -Dm644 "$repo/profile/airootfs/usr/share/applications/synterra-hub.desktop" "$payload/usr/share/applications/synterra-hub.desktop"
install -Dm755 "$repo/target/hub/styles/libSynterraGlass.so" "$payload/usr/lib/qt6/plugins/styles/libSynterraGlass.so"
install -Dm755 "$repo/profile/airootfs/usr/local/bin/synterra-appearance" "$payload/usr/local/bin/synterra-appearance"
install -Dm644 "$repo/profile/airootfs/usr/share/applications/synterra-appearance.desktop" "$payload/usr/share/applications/synterra-appearance.desktop"
install -Dm755 "$repo/target/synaqua/synaqua-shell" "$payload/usr/local/bin/synaqua-shell"
install -Dm755 "$repo/target/synaqua/kcm_synaqua_dock.so" "$payload/usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_synaqua_dock.so"
for helper in start-synaqua; do
    install -Dm755 "$repo/profile/airootfs/usr/local/bin/$helper" "$payload/usr/local/bin/$helper"
done
for helper in synterra-plasma-shell synaqua-autostart; do
    install -Dm755 "$repo/profile/airootfs/usr/local/libexec/$helper" "$payload/usr/local/libexec/$helper"
done
install -Dm644 "$repo/profile/airootfs/usr/share/wayland-sessions/synaqua.desktop" "$payload/usr/share/wayland-sessions/synaqua.desktop"
install -Dm644 "$repo/profile/airootfs/usr/share/applications/synaqua.desktop" "$payload/usr/share/applications/synaqua.desktop"
install -Dm644 "$repo/profile/airootfs/usr/lib/systemd/user/synaqua-shell.service" "$payload/usr/lib/systemd/user/synaqua-shell.service"
install -Dm644 "$repo/profile/airootfs/etc/systemd/user/plasma-plasmashell.service.d/10-synaqua.conf" "$payload/etc/systemd/user/plasma-plasmashell.service.d/10-synaqua.conf"
mkdir -p "$payload/usr/share/plasma/shells"
cp -a "$shell" "$payload/usr/share/plasma/shells/"
cp "$repo/profile/packages.txt" "$dest/airootfs/usr/share/synterra/install-packages.txt"
grep -Ev '^\s*(#|$)' "$repo/profile/packages.txt" >> "$dest/packages.x86_64"
sort -u "$dest/packages.x86_64" -o "$dest/packages.x86_64"
# This file never consumes the host's (possibly Manjaro) repository config.
cp "$repo/profile/pacman.conf" "$dest/pacman.conf"
cp "$repo/profile/pacman.conf" "$dest/airootfs/etc/pacman.conf"
mkdir -p "$dest/airootfs/etc/pacman.d"
printf 'Server = https://geo.mirror.pkgbuild.com/$repo/os/$arch\n' > "$dest/airootfs/etc/pacman.d/mirrorlist"
cat >> "$dest/profiledef.sh" <<'PROFILE'

# Use GRUB for UEFI; retain Archiso's supported Syslinux BIOS fallback.
bootmodes=('bios.syslinux' 'uefi.grub')
iso_name="synterra"
iso_label="SYNTERRA_$(date -u +%Y%m)"
iso_publisher="Synterra"
iso_application="Synterra Glass Live Desktop"
iso_version="1.1.0"
# Zstandard keeps live-image decompression fast and bounds build CPU usage.
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '15' '-b' '1M' '-processors' '4')
file_permissions+=(
  ["/usr/local/bin/synterra-live-setup"]="0:0:755"
  ["/usr/local/bin/synterra-welcome"]="0:0:755"
  ["/usr/local/bin/synterra-install"]="0:0:755"
  ["/usr/local/libexec/synterra-tools"]="0:0:755"
  ["/usr/local/bin/synterra-surf"]="0:0:755"
  ["/usr/local/bin/synterra-hub"]="0:0:755"
  ["/usr/local/bin/synterra-appearance"]="0:0:755"
  ["/usr/local/bin/synaqua-shell"]="0:0:755"
  ["/usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_synaqua_dock.so"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_synaqua_dock.so"]="0:0:755"
  ["/usr/local/bin/start-synaqua"]="0:0:755"
  ["/usr/local/libexec/synterra-plasma-shell"]="0:0:755"
  ["/usr/local/libexec/synaqua-autostart"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/synaqua-shell"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/start-synaqua"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/libexec/synterra-plasma-shell"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/libexec/synaqua-autostart"]="0:0:755"
  ["/etc/skel/Desktop/synterra-home.desktop"]="0:0:755"
  ["/etc/skel/Desktop/synterra-install.desktop"]="0:0:755"
  ["/etc/skel/Desktop/synterra-surf.desktop"]="0:0:755"
  ["/usr/share/synterra/install-overlay/etc/skel/Desktop/synterra-home.desktop"]="0:0:755"
  ["/usr/share/synterra/install-overlay/etc/skel/Desktop/synterra-surf.desktop"]="0:0:755"
  ["/usr/lib/qt6/plugins/styles/libSynterraGlass.so"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/synterra-appearance"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/lib/qt6/plugins/styles/libSynterraGlass.so"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/synterra-hub"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/synterra-welcome"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/bin/synterra-surf"]="0:0:755"
  ["/usr/local/libexec/synterra-recover"]="0:0:755"
  ["/usr/local/libexec/synterra-grub-setup"]="0:0:755"
  ["/usr/share/synterra/install-overlay/usr/local/libexec/synterra-grub-setup"]="0:0:755"
  ["/etc/sudoers.d/10-synterra-live"]="0:0:440"
)
PROFILE
for variant in aurora graphite; do
    wall="$dest/airootfs/usr/share/wallpapers/Synterra-$variant"
    mkdir -p "$wall/contents/images"
    cp "$repo/assets/wallpapers/Prism-$variant-4K.png" "$wall/contents/images/3840x2160.png"
    printf '{"KPlugin":{"Id":"Synterra-%s","Name":"Synterra Prism %s","License":"LicenseRef-User-Supplied"}}\n' "$variant" "$variant" > "$wall/metadata.json"
done
mkdir -p "$payload/usr/share/wallpapers"
cp -a "$dest/airootfs/usr/share/wallpapers/Synterra-aurora" "$dest/airootfs/usr/share/wallpapers/Synterra-graphite" "$payload/usr/share/wallpapers/"
system="$dest/airootfs/etc/systemd/system"
mkdir -p "$system/multi-user.target.wants"
# NetworkManager owns interfaces; keep resolved for releng's resolver symlink.
for service in iwd sshd systemd-networkd choose-mirror; do
    rm -f "$system/multi-user.target.wants/$service.service"
done
rm -f "$system/dbus-org.freedesktop.network1.service" \
    "$system/sockets.target.wants/systemd-networkd.socket" \
    "$system/network-online.target.wants/systemd-networkd-wait-online.service"
ln -sfn /dev/null "$system/systemd-networkd.service"
ln -sfn /dev/null "$system/systemd-networkd.socket"
ln -sfn /usr/lib/systemd/system/NetworkManager.service "$system/multi-user.target.wants/NetworkManager.service"
ln -sfn /usr/lib/systemd/system/bluetooth.service "$system/multi-user.target.wants/bluetooth.service"
ln -sfn /usr/lib/systemd/system/vmtoolsd.service "$system/multi-user.target.wants/vmtoolsd.service"
ln -sfn /etc/systemd/system/synterra-live-setup.service "$system/multi-user.target.wants/synterra-live-setup.service"
ln -sfn /usr/lib/systemd/system/sddm.service "$system/display-manager.service"
ln -sfn /usr/lib/systemd/system/graphical.target "$system/default.target"
# Drop upstream root-console autologin; desktop autologin uses the live user.
rm -f "$system/getty@tty1.service.d/autologin.conf"
for bootdir in grub efiboot syslinux; do
    if [[ -d $dest/$bootdir ]]; then
        find "$dest/$bootdir" -type f \( -name '*.cfg' -o -name '*.conf' \) -exec sed -i 's/Arch Linux/Synterra/g; s/Synterra install medium/Synterra Live Desktop/g' {} +
    fi
done
echo "Prepared $dest"
