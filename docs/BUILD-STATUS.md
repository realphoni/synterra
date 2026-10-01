# Build status — 2026-10-01

## Bootstrap signature correction

The first guest build downloaded the Arch bootstrap but stopped before extraction: `invalid packet (ctb=2d)` and `No public key`. The builder passed pacman's armored OpenPGP key file as a GPG database. It now imports that file into the temporary GPG home before verifying the detached signature. Signature verification remains mandatory.

Regression test passed locally with the existing GPG installation: an armored key file imports, a valid detached signature succeeds, and altered data fails. Bash syntax and source validation also passed. The corrected full build still needs to run in Manjaro.

The repository is now public. Existing clones can update using `git pull --ff-only`.

```bash
python scripts/test-bootstrap-signature.py
```

## Initial source validation

Completed:

- Arch-based source and isolated Manjaro-to-Arch bootstrap builder.
- Original Plasma glass style, Aurorae window frames, palette, launcher orb and initial desktop layout.
- Supplied Aurora and Graphite wallpapers copied intact; SHA-256 hashes match the originals; both are 3840 × 2160.
- Bash parser checks passed for all five shell scripts (existing MSYS Bash, syntax only).
- Source validator passed: JSON metadata, ten SVG assets, required frame and blur-mask IDs, PNG dimensions, Linux line endings, palette and package manifest.
- Existing Manjaro VM started with VMware Workstation's `vmrun`; VMware Tools reports `running`.
- Read-only `Synterra` source share added and enabled for the running VM session.

Pending:

- User will run the build command in the Manjaro guest. VMware guest commands require authentication; none was supplied or bypassed.
- Full bootstrap/package resolution, archiso build, ISO boot, live desktop visuals and BIOS/UEFI validation have not run.
- No ISO, installed-system image or graphical installer has been produced.

Computer-use window inspection failed with `FrameArrived timed out: timed out waiting on channel`, then `window capture timed out: timed out waiting on channel`. Existing VMware CLI was used to start and inspect the VM instead. No WSL was installed or used.

Validation commands:

```bash
python scripts/validate.py
for f in scripts/*.sh profile/airootfs/usr/local/bin/*; do bash -n "$f"; done
```

Build in Manjaro after copying the source to `~/synterra`:

```bash
cd ~/synterra
sudo bash scripts/build-manjaro.sh
```

Expected result: `~/synterra/out/synterra-*.iso`, build logs and `SHA256SUMS`. Source checks do not prove that the ISO builds or that the theme renders correctly in Plasma.
