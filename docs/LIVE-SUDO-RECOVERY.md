# Live ISO sudo recovery

The published Synterra 1.1.0 ISO (Build 1170) contains an image permission defect:
`/etc/sudoers.d` is mode `0777`. Sudo rejects that directory and therefore does
not load the live account's rule. This is unrelated to the administrator account
you select in Archinstall. The image's `/` and `/etc` directories also inherited
Windows copy permissions.

## Recover the existing live session

These instructions apply only to the **1.1 live ISO**, whose root account has an
empty password. They do not apply to an installed system's password-protected
root account.

1. Press **Ctrl+Alt+F3** to open a text console. In VMware, send that key
   combination to the guest.
2. Log in as `root`. If prompted for a password, press Enter.
3. Run:

   ```bash
   chown root:root / /etc /etc/sudoers /etc/sudoers.d /etc/sudoers.d/10-synterra-live
   chmod 755 / /etc /etc/sudoers.d
   chmod 440 /etc/sudoers /etc/sudoers.d/10-synterra-live
   visudo -c
   runuser -u live -- sudo -n -- id -u
   ```

   The last command should print `0`. Return to the desktop with Ctrl+Alt+F1
   (or F2, depending on the session), then open the installer again.

The live filesystem is temporary, so the repair is lost when that ISO restarts.
It does not reformat disks, reinstall the system, or change installed accounts.
If an installation is already mounted, use the installer's continuation/recovery
path rather than restarting partitioning.

## Build correction

Build 2010 explicitly sets root ownership and `0755` on `/`, `/etc` and
`/etc/sudoers.d`, and `0440` on both policy files. Overlay normalization removes
DrvFS's synthetic `0777` modes. Live setup validates sudoers before the desktop
starts. Build tests check actual elevation as `live`, without opening a PAM user
session ahead of SDDM. Installed payloads do
not contain the live account, autologin or passwordless sudo rule.

`scripts/test-live-sudo.sh LIVE_ROOT_FILESYSTEM` checks the built modes, runs real
sudo in an isolated overlay, reproduces the old failure, tests startup repair,
and verifies that an unrelated account gets no access. The full Archiso build
runs this check before reporting success.

The full build also tests Archiso's actual overlay-copy function, ensuring that
regular data stays `0644` and live/installed session helpers remain executable.

The existing GitHub 1.1.0 ISO has not been replaced by these source changes.
