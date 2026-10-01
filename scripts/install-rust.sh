#!/usr/bin/env bash
set -euo pipefail
[[ $EUID != 0 ]] || { echo 'Run as your normal Manjaro user, without sudo.' >&2; exit 1; }
command -v curl >/dev/null || { echo 'curl is required.' >&2; exit 1; }
installer=$(mktemp)
trap 'rm -f "$installer"' EXIT
curl --proto '=https' --tlsv1.2 --fail --location https://sh.rustup.rs -o "$installer"
sh "$installer" -y --profile minimal --no-modify-path
echo 'Rust installed. Run: source "$HOME/.cargo/env"'
