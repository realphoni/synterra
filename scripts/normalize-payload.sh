#!/usr/bin/env bash
set -Eeuo pipefail
payload=$(realpath -e -- "${1:?Installed payload directory required}")
[[ $payload != / && -d $payload/etc/skel && -f $payload/usr/share/synterra/os-release ]] || exit 1
if (( EUID == 0 )); then chown -hR 0:0 "$payload"; fi
# DrvFS copies can carry Windows' synthetic 0777 modes. Use explicit Unix modes
# before either packaging these files or copying them into an installation.
find "$payload" -type d -exec chmod 755 {} +
find "$payload" -type f -exec chmod 644 {} +
for directory in usr/local/bin usr/local/libexec etc/skel/Desktop; do
    [[ ! -d $payload/$directory ]] || find "$payload/$directory" -type f -exec chmod 755 {} +
done
find "$payload/usr/lib" -type f -name '*.so' -exec chmod 755 {} +
