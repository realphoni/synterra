# Build status — 2026-10-01

The first Synterra development ISO was built successfully with Archiso in the dedicated Arch Linux WSL distribution `SynterraBuild`. WSL 3.0.1 is installed and operational.

- Image: `synterra-0.1.20261001-x86_64.iso`.
- Size: 2,460,696,576 bytes.
- SHA-256: `01cbcdf48bf534657729d96732fce763ee9fd45c6ffafa3a7b070ff486eeeb56`.
- Copied from the Linux build filesystem to `E:\tuff\out`; verification of the copied ISO passed.
- Rust source validation and package installation passed; Archiso completed ISO creation.

The release contains two ISO parts because the full image exceeds GitHub's per-asset limit. `SHA256SUMS.parts` verifies the parts and `SHA256SUMS` verifies the assembled image. See the README for assembly instructions.

## Build corrections

Synterra's `os-release` is stored in `/usr/share/synterra/` and applied by a post-transaction pacman hook. This avoids a conflict with Arch's `filesystem` package during installation and reapplies branding on upgrades.

The Manjaro builder imports the armored Arch keyring into a temporary GPG home before verifying the bootstrap signature. Signature verification remains mandatory. The Rust GPG regression accepted a valid signature and rejected tampering.

## Source validation

Artwork generation, source validation and the GPG regression passed using Rust on Windows. The Linux source validator passed during the WSL build. Dependencies are pinned in `Cargo.lock`; build orchestration uses Bash.

The original Aurora and Graphite wallpapers remain unchanged at 3840 × 2160. The profile includes the Aero-inspired Plasma style, Aurorae frames, palette, launcher orb and bottom panel.

## Pending validation

The ISO has not been boot-tested. VMware BIOS/UEFI boot, Plasma autologin, desktop appearance, networking, audio, guest resize and clipboard still require testing. Compilation and source checks do not prove runtime behavior.

This is a development live image. A finished graphical installer, installed-system workflow and release signing remain future work.
