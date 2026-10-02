#!/usr/bin/env bash
# Repair a mounted Prism 0.2 installation without repartitioning or formatting.
set -Eeuo pipefail
trap 'echo "Recovery stopped at line $LINENO. Keep the live ISO running; inspect the error above." >&2' ERR
target=/mnt/synterra-install
payload=/usr/share/synterra/install-overlay
[[ $EUID == 0 && -d /run/archiso ]] || { echo 'Run with sudo from the Synterra live ISO.' >&2; exit 1; }
[[ $(readlink -f "$target") == "$target" ]] || { echo 'Refusing a symlinked target.' >&2; exit 1; }
mountpoint -q "$target" || { echo 'The installed root is no longer mounted. Stop here; do not format or reinstall.' >&2; exit 1; }
source_device=$(findmnt -n -o SOURCE --mountpoint "$target")
[[ $source_device == /dev/* ]] || { echo "Unexpected root source: $source_device" >&2; exit 1; }
[[ -f $target/etc/passwd && -f $target/usr/bin/pacman && -d $payload/etc/skel ]] || { echo 'Installed base or Synterra payload is missing.' >&2; exit 1; }
[[ ! -L $target/home ]] || { echo 'Refusing a symlinked home directory.' >&2; exit 1; }
arch-chroot "$target" pacman -Q linux plasma-desktop sddm networkmanager >/dev/null
if [[ -d /sys/firmware/efi ]]; then
    # Require the installed EFI partition to remain mounted before generating fstab.
    mounted_esp=
    for esp in "$target/boot" "$target/efi" "$target/boot/efi"; do
        if mountpoint -q "$esp" && [[ $(findmnt -n -o FSTYPE --mountpoint "$esp") == vfat ]]; then
            mounted_esp=$esp
        fi
    done
    [[ -n $mounted_esp ]] || { echo 'The installed EFI partition is not mounted. Mount it before recovery; do not reformat it.' >&2; exit 1; }
    bootloader=$(find "$mounted_esp" -type f -iname '*.efi' -print -quit)
    [[ -n $bootloader ]] || { echo 'No EFI bootloader found on the mounted installation.' >&2; exit 1; }
fi
echo "Repair Synterra on $source_device, mounted at $target."
echo 'This will select SDDM, apply Synterra desktop defaults and regenerate fstab.'
echo 'No partitions will be created or formatted. Existing fstab will be backed up.'
read -r -p 'Type REPAIR to continue: ' confirmation
[[ $confirmation == REPAIR ]] || { echo 'Cancelled.'; exit 0; }
mapfile -t users < <(awk -F: '$3 >= 1000 && $3 < 60000 && $1 != "live" && $6 == "/home/" $1 {print $1}' "$target/etc/passwd")
if (( ${#users[@]} == 0 )); then
    read -r -p 'Create an administrator account. Username: ' username
    [[ $username =~ ^[a-z_][a-z0-9_-]{0,31}$ && $username != live ]] || { echo 'Invalid username.' >&2; exit 1; }
    arch-chroot "$target" useradd -m -s /bin/bash -G wheel "$username"
    arch-chroot "$target" passwd "$username"
    printf '%%wheel ALL=(ALL:ALL) ALL\n' | install -m440 /dev/stdin "$target/etc/sudoers.d/20-synterra-admin"
    arch-chroot "$target" visudo -c
    users=("$username")
fi
for username in "${users[@]}"; do
    [[ $username =~ ^[a-zA-Z0-9_-]+$ && -d $target/home/$username && ! -L $target/home/$username ]] || { echo 'Unsafe or missing user home.' >&2; exit 1; }
done
cp -a "$payload/." "$target/"
cp "$target/usr/share/synterra/os-release" "$target/usr/lib/os-release"
for username in "${users[@]}"; do
    cp -a "$payload/etc/skel/." "$target/home/$username/"
    primary_group=$(arch-chroot "$target" id -gn "$username")
    arch-chroot "$target" chown -R "$username:$primary_group" "/home/$username"
done
mkdir -p "$target/etc/sddm.conf.d"
printf '[Theme]\nCurrent=breeze\n' > "$target/etc/sddm.conf.d/10-synterra.conf"
previous_manager=$(readlink "$target/etc/systemd/system/display-manager.service" || true)
if [[ ${previous_manager##*/} == cosmic-greeter.service ]]; then
    systemctl --root="$target" disable cosmic-greeter
fi
systemctl --root="$target" --force enable sddm
systemctl --root="$target" enable NetworkManager vmtoolsd
systemctl --root="$target" set-default graphical.target
if [[ -f $target/etc/fstab ]]; then
    cp -a "$target/etc/fstab" "$target/etc/fstab.before-synterra-recovery-$(date +%s)"
fi
new_fstab=$(mktemp /run/synterra-fstab.XXXXXX)
genfstab -U "$target" > "$new_fstab"
grep -Eq '^[^#]+[[:space:]]+/[[:space:]]+' "$new_fstab"
install -m644 "$new_fstab" "$target/etc/fstab"
rm -f "$new_fstab"
if [[ -f $target/boot/grub/grub.cfg ]]; then
    if [[ -x $target/usr/local/libexec/synterra-grub-setup && -f $target/usr/share/grub/themes/SynterraPrism/theme.txt ]]; then
        arch-chroot "$target" /usr/local/libexec/synterra-grub-setup
    else
        # Older live-image payloads do not contain the Prism 0.5 theme helper.
        arch-chroot "$target" grub-mkconfig -o /boot/grub/grub.cfg
    fi
fi
sync
echo 'Synterra recovery finished. Shut down, remove the ISO and boot the installed disk.'
