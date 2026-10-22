#!/usr/bin/env bash
# Exercise the installed Archiso implementation, including its recursive-path rule.
set -Eeuo pipefail
[[ $EUID == 0 ]] || exit 1
profile=$(realpath -e -- "${1:?Prepared Archiso profile required}")
[[ -f $profile/profiledef.sh && -f $profile/airootfs/usr/share/synterra/os-release ]] || exit 1
declare -A file_permissions=()
source "$profile/profiledef.sh"
pacstrap_dir=$(mktemp -d /var/tmp/synterra-profile-test.XXXXXX)
trap 'rm -rf --one-file-system -- "$pacstrap_dir"' EXIT
umask 022
# Use the real upstream function, but run its simple filesystem commands directly
# within this temporary destination instead of creating a user namespace.
_unshare() { "$@"; }
_msg_info() { :; }
_msg_warning() { :; }
_msg_error() { echo "$1" >&2; exit 1; }
implementation=$(sed -n '/^_make_custom_airootfs()/,/^}/p' "$(command -v mkarchiso)")
[[ $implementation == _make_custom_airootfs* ]] || { echo 'Archiso copy implementation changed; inspect before building.' >&2; exit 1; }
source /dev/stdin <<< "$implementation"
_make_custom_airootfs
for directory in '' /etc /etc/sudoers.d; do
    [[ $(stat -c '%u:%g:%a' "$pacstrap_dir/$directory") == 0:0:755 ]]
done
[[ $(stat -c '%u:%g:%a' "$pacstrap_dir/etc/sudoers.d/10-synterra-live") == 0:0:440 ]]
[[ $(stat -c '%u:%g:%a' "$pacstrap_dir/usr/local/bin/synterra-live-setup") == 0:0:755 ]]
[[ $(stat -c '%u:%g:%a' "$pacstrap_dir/usr/share/synterra/os-release") == 0:0:644 ]]
for prefix in '' /usr/share/synterra/install-overlay; do
    for directory in usr/local/bin usr/local/libexec; do
        [[ -d $pacstrap_dir$prefix/$directory ]]
        while IFS= read -r -d '' helper; do
            [[ $(stat -c '%u:%g:%a' "$helper") == 0:0:755 ]] || { echo "Non-executable or unowned helper: ${helper#"$pacstrap_dir"}" >&2; exit 1; }
        done < <(find "$pacstrap_dir$prefix/$directory" -type f -print0)
    done
done
echo 'PASS: actual Archiso copy and permission overrides; policy 0440, directories 0755, ordinary data 0644.'
