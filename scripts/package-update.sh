#!/usr/bin/env bash
set -Eeuo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
payload=$(realpath -e -- "${1:?Usage: package-update.sh INSTALLED_PAYLOAD OUTPUT_DIRECTORY}")
destination=${2:?Output directory required}
[[ -f $payload/usr/share/synterra/os-release && -x $payload/usr/local/bin/synterra-updates ]] || { echo 'Complete installed-system payload required.' >&2; exit 1; }
version=$(sed -n 's/^VERSION_ID=//p' "$payload/usr/share/synterra/os-release" | tr -d '"')
build=$(sed -n 's/^BUILD_ID=//p' "$payload/usr/share/synterra/os-release")
[[ $version =~ ^[0-9]+\.[0-9]+$ && $build =~ ^[0-9]+$ ]] || { echo 'Invalid release version.' >&2; exit 1; }
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/payload" "$stage/home" "$destination"
cp -a "$payload/." "$stage/payload/"
install -Dm644 "$repo/LICENSE" "$stage/payload/usr/share/licenses/synterra-release/LICENSE"
# Package only the installed desktop. Never ship accounts, live autologin,
# passwordless sudo, user homes or a second embedded installer payload.
[[ ! -e $stage/payload/etc/passwd && ! -e $stage/payload/etc/sudoers.d && ! -e $stage/payload/home && ! -e $stage/payload/usr/share/synterra/install-overlay ]]
bash "$repo/scripts/normalize-payload.sh" "$stage/payload"
find "$stage/payload" -depth -mindepth 1 -type d -empty -delete
cat > "$stage/PKGBUILD" <<PKG
pkgname=synterra-release
pkgver=$version.$build
pkgrel=1
pkgdesc='Synterra desktop and distribution release files'
arch=('x86_64')
url='https://github.com/realphoni/synterra'
license=('MIT')
depends=('qt6-base' 'qt6-webengine' 'qt6-declarative' 'qt6-wayland' 'libarchive' 'plasma-workspace' 'plasma-desktop' 'layer-shell-qt' 'kcmutils' 'kio' 'kservice' 'kglobalaccel' 'kwindowsystem' 'konsole' 'sudo' 'grub')
options=('!strip' '!debug')
package() { cp -a "\$startdir/payload/." "\$pkgdir/"; chown -R root:root "\$pkgdir"; }
PKG
if (( EUID == 0 )); then
    chown -R nobody:nobody "$stage"
    (cd "$stage" && runuser -u nobody -- env HOME="$stage/home" makepkg --nodeps --nocheck --nosign --noconfirm)
else
    (cd "$stage" && makepkg --nodeps --nocheck --nosign --noconfirm)
fi
package="synterra-release-$version.$build-1-x86_64.pkg.tar.zst"
install -m644 "$stage/$package" "$destination/$package"
"$repo/target/updater/synterra-update-manifest" "$destination/$package" "$destination/synterra-update.json" "$version" "$build"
(cd "$destination" && sha256sum "$package" synterra-update.json > SHA256SUMS.update)
echo "Prepared $package and synterra-update.json; attach both to v$version.$build."
