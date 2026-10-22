#!/usr/bin/env bash
set -Eeuo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(realpath -e -- "${1:?Prepared airootfs directory required}")
[[ $root != / && -f $root/usr/share/synterra/os-release ]] || exit 1
chmod 755 "$root"
source="$repo/profile/airootfs"
while IFS= read -r -d '' path; do
    destination="$root/${path#"$source/"}"
    chown -h 0:0 "$destination"
    if [[ -d $path && ! -L $path ]]; then chmod 755 "$destination"
    elif [[ -f $path && ! -L $path ]]; then chmod 644 "$destination"
    fi
done < <(find "$source" -mindepth 1 -print0)
for directory in usr/local/bin usr/local/libexec etc/skel/Desktop; do
    find "$root/$directory" -maxdepth 1 -type f -exec chmod 755 {} +
done
# DrvFS copies can preserve 0777 directories even when the rule itself is 0440.
# Sudo refuses such an include directory. Normalize both ownership and modes.
chown 0:0 "$root" "$root/etc" "$root/etc/sudoers.d" "$root/etc/sudoers.d/10-synterra-live"
chmod 755 "$root/etc" "$root/etc/sudoers.d"
chmod 440 "$root/etc/sudoers.d/10-synterra-live"
