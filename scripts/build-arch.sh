#!/usr/bin/env bash
set -Eeuo pipefail
[[ $EUID == 0 ]] || { echo 'Run with sudo inside Linux.' >&2; exit 1; }
source /etc/os-release
[[ ${ID:-} == arch ]] || { echo 'Use build-manjaro.sh on Manjaro; this builder requires Arch.' >&2; exit 1; }
command -v mkarchiso >/dev/null || { echo 'Install archiso first.' >&2; exit 1; }
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
stamp=$(date -u +%Y%m%dT%H%M%SZ)
mkdir -p "$repo/build" "$repo/out"
run=$(mktemp -d "$repo/build/$stamp.XXXXXX")
bash "$repo/scripts/prepare-profile.sh" "$run/profile"
mkarchiso -v -w "$run/work" -o "$repo/out" "$run/profile" 2>&1 | tee "$repo/out/build-$stamp.log"
find "$repo/out" -maxdepth 1 -name 'synterra-*.iso' -type f -exec sha256sum {} \; > "$repo/out/SHA256SUMS"
echo "Synterra ISO and checksums: $repo/out"
