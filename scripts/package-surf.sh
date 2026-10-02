#!/usr/bin/env bash
set -Eeuo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
version=$(sed -n 's/^project(synterra_surf VERSION \([^ ]*\).*/\1/p' "$repo/browser/CMakeLists.txt")
[[ $(uname -m) == x86_64 && -n $version ]] || { echo 'Use an x86_64 Arch build host.' >&2; exit 1; }
[[ -x $repo/target/surf/synterra-surf ]] || { echo 'Compile Surf before packaging it.' >&2; exit 1; }
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
name="synterra-surf-$version-linux-x86_64"
mkdir -p "$stage/$name" "$repo/out"
install -m755 "$repo/target/surf/synterra-surf" "$stage/$name/synterra-surf"
install -m644 "$repo/profile/airootfs/usr/share/applications/synterra-surf.desktop" "$stage/$name/"
install -m644 "$repo/profile/airootfs/usr/share/icons/hicolor/scalable/apps/synterra-surf.svg" "$stage/$name/"
install -m644 "$repo/LICENSE" "$stage/$name/"
cat > "$stage/$name/README.txt" <<'README'
Synterra Surf for x86_64 Arch/Synterra systems

This archive contains the native browser only. It requires the current Arch
qt6-webengine and qt6-base packages; it is not a standalone Windows executable.
Verify the archive against the release's SHA256SUMS.surf before extracting.

To update Surf on an installed Synterra system, from this extracted directory:
  sudo pacman -Syu qt6-webengine qt6-base
  sudo install -Dm755 synterra-surf /usr/local/bin/synterra-surf
  sudo install -Dm644 synterra-surf.desktop /usr/share/applications/synterra-surf.desktop
  sudo install -Dm644 synterra-surf.svg /usr/share/icons/hicolor/scalable/apps/synterra-surf.svg

Close Surf before updating. Your existing profile and bookmarks are retained.
Start a private window with: synterra-surf --private
README
tar -C "$stage" -czf "$repo/out/$name.tar.gz" "$name"
(cd "$repo/out" && sha256sum "$name.tar.gz" > SHA256SUMS.surf)
echo "Packaged $name.tar.gz"
