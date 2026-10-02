#!/usr/bin/env bash
set -Eeuo pipefail
[[ $EUID == 0 ]] || { echo 'Run with sudo inside the Manjaro VMware guest.' >&2; exit 1; }
[[ $(uname -m) == x86_64 ]] || { echo 'x86_64 required.' >&2; exit 1; }
for tool in curl gpg tar zstd mount umount chroot mountpoint; do
    command -v "$tool" >/dev/null || { echo "Missing prerequisite: $tool" >&2; exit 1; }
done
keyring=/usr/share/pacman/keyrings/archlinux.gpg
[[ -r $keyring ]] || { echo "Install archlinux-keyring on the builder; missing $keyring" >&2; exit 1; }
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build=$(mktemp -d /var/tmp/synterra-build.XXXXXX)
root="$build/root.x86_64"
mkdir -p "$build/gnupg" "$repo/out"
chmod 700 "$build/gnupg"
exec > >(tee "$repo/out/bootstrap-$(date -u +%Y%m%dT%H%M%SZ).log") 2>&1
echo "Arch build root: $build"
base=https://geo.mirror.pkgbuild.com/iso/latest
archive=archlinux-bootstrap-x86_64.tar.zst
curl --fail --location --retry 3 "$base/$archive" -o "$build/$archive"
curl --fail --location --retry 3 "$base/$archive.sig" -o "$build/$archive.sig"
# Pacman's key file may be ASCII-armored OpenPGP data, not a GPG database.
# Import into our isolated key database before verifying; never modify the host's.
gpg --no-options --homedir "$build/gnupg" --batch --import "$keyring"
gpg --no-options --homedir "$build/gnupg" --batch --verify "$build/$archive.sig" "$build/$archive"
tar --zstd -xpf "$build/$archive" -C "$build"
rm -f "$root/etc/resolv.conf"
cp -L /etc/resolv.conf "$root/etc/resolv.conf"
printf 'Server = https://geo.mirror.pkgbuild.com/$repo/os/$arch\n' > "$root/etc/pacman.d/mirrorlist"
mkdir -p "$root/root/synterra"
tar -C "$repo" -cf - scripts src browser Cargo.toml Cargo.lock profile assets README.md LICENSE .gitattributes | tar -C "$root/root/synterra" -xf -
mounted=()
cleanup() {
    local i
    for ((i=${#mounted[@]}-1; i>=0; i--)); do
        umount -R "${mounted[i]}" || echo "Unmount manually before removing: ${mounted[i]}" >&2
    done
    echo "Build root retained at $build"
}
trap cleanup EXIT
mount --rbind /dev "$root/dev"; mounted+=("$root/dev")
mount --make-rslave "$root/dev"
mount -t proc proc "$root/proc"; mounted+=("$root/proc")
mount --rbind /sys "$root/sys"; mounted+=("$root/sys")
mount --make-rslave "$root/sys"
mount -t tmpfs tmpfs "$root/run"; mounted+=("$root/run")
chroot "$root" /bin/bash -euxc '
    pacman-key --init
    pacman-key --populate archlinux
    pacman -Syu --noconfirm archlinux-keyring archiso rust gcc cmake ninja pkgconf qt6-webengine
    bash /root/synterra/scripts/build-arch.sh
'
cp -a "$root/root/synterra/out/." "$repo/out/"
echo "ISO copied to $repo/out"
