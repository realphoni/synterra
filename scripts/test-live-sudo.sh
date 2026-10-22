#!/usr/bin/env bash
# Exercise the real sudo policy in an isolated writable overlay, never on the host.
set -Eeuo pipefail
[[ $EUID == 0 ]] || { echo 'Run as root on the Linux build host.' >&2; exit 1; }
root=$(realpath -e -- "${1:?Usage: test-live-sudo.sh LIVE_ROOT_FILESYSTEM}")
[[ $root != / && -x $root/usr/local/bin/synterra-live-setup && -x $root/usr/bin/sudo ]] || exit 1
for entry in / /etc /etc/sudoers.d; do
    [[ $(stat -c '%u:%g:%a' "$root/$entry") == 0:0:755 ]] || { echo "Unsafe directory permissions: $entry" >&2; exit 1; }
done
for entry in /etc/sudoers /etc/sudoers.d/10-synterra-live; do
    [[ $(stat -c '%u:%g:%a' "$root/$entry") == 0:0:440 ]] || { echo "Unsafe sudo policy permissions: $entry" >&2; exit 1; }
done
# A mount namespace contains the test mounts even if the process is interrupted.
exec unshare --mount --propagation private -- bash -s -- "$root" <<'TEST'
set -Eeuo pipefail
root=$1
stage=$(mktemp -d /var/tmp/synterra-sudo-test.XXXXXX)
cleanup() {
    if mountpoint -q "$stage/root"; then
        umount -R "$stage/root" || { echo "Test mounts remain at $stage; leaving them for inspection." >&2; return; }
    fi
    rm -rf --one-file-system -- "$stage"
}
trap cleanup EXIT
mkdir "$stage/upper" "$stage/work" "$stage/root"
mount -t overlay overlay -o "lowerdir=$root,upperdir=$stage/upper,workdir=$stage/work" "$stage/root"
mount --bind /dev "$stage/root/dev"
mount --bind /dev/pts "$stage/root/dev/pts"
mount -t proc proc "$stage/root/proc"
mkdir -p "$stage/root/run/archiso"
chroot "$stage/root" /usr/local/bin/synterra-live-setup
[[ $(chroot "$stage/root" runuser -u live -- sudo -n -- id -u) == 0 ]]
# Reproduce the released 1.1 defect, then exercise startup's defensive repair.
chmod 777 "$stage/root/etc/sudoers.d"
if chroot "$stage/root" runuser -u live -- sudo -n -- id -u > "$stage/rejected.log" 2>&1; then
    echo 'Regression fixture unexpectedly accepted a world-writable include directory.' >&2
    exit 1
fi
grep -q 'world writable' "$stage/rejected.log"
chroot "$stage/root" /usr/local/bin/synterra-live-setup
[[ $(stat -c '%u:%g:%a' "$stage/root/etc/sudoers.d") == 0:0:755 ]]
[[ $(chroot "$stage/root" runuser -u live -- sudo -n -- id -u) == 0 ]]
chroot "$stage/root" visudo -c
# The live rule must not grant a different installed-style account access.
chroot "$stage/root" useradd -m -s /bin/bash synterra-policy-test
if chroot "$stage/root" runuser -u synterra-policy-test -- sudo -n -- id -u >/dev/null 2>&1; then
    echo 'The live policy unexpectedly granted a different account access.' >&2
    exit 1
fi
echo 'PASS: root-owned policy, live sudo, reproduced 1.1 failure, startup repair, and no grant to other accounts.'
TEST
