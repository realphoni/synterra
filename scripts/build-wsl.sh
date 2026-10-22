#!/usr/bin/env bash
set -Eeuo pipefail
[[ $EUID == 0 ]] || { echo 'Run this script as root in the Arch WSL build distribution.' >&2; exit 1; }
source /etc/os-release
[[ ${ID:-} == arch ]] || { echo 'This WSL builder requires Arch Linux.' >&2; exit 1; }
source_dir=${1:-/mnt/e/tuff}
destination=${2:-/mnt/e/tuff/out}
[[ -f $source_dir/Cargo.toml ]] || { echo "Synterra source missing at $source_dir" >&2; exit 1; }
repo=/root/synterra
mkdir -p "$repo" "$destination"
# Build on the Linux filesystem; do not put archiso working files on DrvFS.
for item in scripts src browser desktop synaqua updater Cargo.toml Cargo.lock profile assets README.md LICENSE .gitattributes; do
    cp -a "$source_dir/$item" "$repo/"
done
# Remove the obsolete pre-install branding overlay from earlier source copies.
rm -f "$repo/profile/airootfs/usr/lib/os-release"
rm -f "$repo/profile/airootfs/etc/skel/.config/fastfetch/config.jsonc" \
    "$repo/profile/airootfs/usr/share/synterra/fastfetch-logo.txt"
pacman-key --init
pacman-key --populate archlinux
pacman -Syu --noconfirm archlinux-keyring archiso fakeroot rust gcc cmake ninja pkgconf qt6-webengine qt6-svg qt6-declarative qt6-wayland layer-shell-qt kservice kio kglobalaccel kwindowsystem kcmutils kcoreaddons kirigami plasma-workspace plasma-desktop grub librsvg ttf-dejavu
cd "$repo"
cargo run --locked -- validate
bash scripts/build-arch.sh
read -r digest image < "$repo/out/SHA256SUMS"
[[ $image == synterra-*.iso && $image != */* ]] || { echo 'Unexpected ISO checksum entry.' >&2; exit 1; }
cp -uv "$repo/out/$image" "$repo/out/SHA256SUMS" "$destination/"
release_version=$(sed -n 's/^VERSION_ID=//p' "$repo/profile/airootfs/usr/share/synterra/os-release" | tr -d '"')
release_build=$(sed -n 's/^BUILD_ID=//p' "$repo/profile/airootfs/usr/share/synterra/os-release")
cp -uv "$repo/out/synterra-release-$release_version.$release_build-1-x86_64.pkg.tar.zst" \
    "$repo/out/synterra-update.json" "$repo/out/SHA256SUMS.update" "$destination/"
find "$repo/out" -maxdepth 1 -type f -name '*.log' -exec cp -uv {} "$destination/" \;
echo "Synterra ISO copied to $destination"
