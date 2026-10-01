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
for item in scripts src Cargo.toml Cargo.lock profile assets README.md LICENSE .gitattributes; do
    cp -a "$source_dir/$item" "$repo/"
done
# Remove the obsolete pre-install branding overlay from earlier source copies.
rm -f "$repo/profile/airootfs/usr/lib/os-release"
pacman-key --init
pacman-key --populate archlinux
pacman -Syu --noconfirm archlinux-keyring archiso rust
cd "$repo"
cargo run --locked -- validate
bash scripts/build-arch.sh
find "$repo/out" -maxdepth 1 -type f \( -name '*.iso' -o -name '*.log' -o -name SHA256SUMS \) -exec cp -v {} "$destination/" \;
echo "Synterra ISO copied to $destination"
