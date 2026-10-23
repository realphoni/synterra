# Build status — 2026-10-23

Synterra 2.0 (Variscite), Build 2020 is a [development prerelease](https://github.com/realphoni/synterra/releases/tag/v2.0.2020) focused on Surf
1.2.0. The released Build 2010 remains
available, with its validation recorded in [the archived status](BUILD-STATUS-2010.md).

## Browser changes

The native notice strip provides information, warnings, source links, per-tab
dismissal/restore and cycling for multiple matches. It follows the active tab,
has light/dark styling and wraps on smaller windows. Settings can disable all
notices, search-topic matching or playful notes independently. Matching uses
local domain/search rules and sends no extra notice requests. The 21-topic
catalog includes a stable rare Ace Attorney variant. The requested attribution
prefixes are removed. See [the notice guide](SURF-NOTICES.md).

Build 2020 also updates the visible distribution build in Surf, Hub, Synaqua,
the installer, updater, sign-in screen and boot artwork. It retains the Variscite
wallpaper, Glass modes and the Build 2010 setup-permission correction.

## Validation

The native notice tests and the full Qt/Chromium browser smoke test passed as
an unprivileged user with sandboxing enabled. The tests cover source buttons,
per-tab/private dismissal, restore, warning priority, cycling, preferences,
stable rare copy, light/dark/narrow layouts, and the existing JavaScript,
download, PDF, popup, DevTools and page-recovery checks. External requests,
including automatic favicons, are blocked during the fixture tests.

Rust validation and installer regressions passed. The actual Archiso permission
overrides and real live-sudo regression also passed against the staged root.
The final ISO's root/policy permissions, branding and filesystem checksum passed.
The exact tested browser binary is present in both live and installed payloads,
the desktop update package and the browser-only archive. Its SHA-256 is
`2d2a3f1003680811320e89d38d5d36b6ae519ae8494e7f6a1d7be646de832a32`.

BIOS and UEFI boot menus from the final ISO were captured and visually checked;
both show Build 2020 and the Variscite artwork. A fresh full desktop boot and
installation cycle were not rerun for this build. The browser itself was
visually checked under Xvfb; the previous full desktop boot is recorded in the
Build 2010 status.

## Release artifacts

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `synterra-2.0.2020-x86_64.iso` | 2,580,217,856 | `ed07f25ee0eb69b5d56553e2ddfee0cf571d45e6d32e9fb47b5e58605ae6d7e4` |
| `synterra-surf-1.2.0-linux-x86_64.tar.gz` | 186,520 | `8a1af8de32a458f33ee4414f7e78627fbf38867d4aa50bd864c77bcae812a1d0` |
| `synterra-release-2.0.2020-1-x86_64.pkg.tar.zst` | 6,767,646 | `78fd9fa2bd0dacabec8fd3e0d4506c531fd6607f315bc129eca4050850df67c3` |

The artifacts are under `out/`. ISO/browser checksums are `SHA256SUMS.2020`
and `SHA256SUMS.surf`. The desktop package, canonical `synterra-update.json`
manifest and `SHA256SUMS.update` are also grouped in `out/2020-work/`.
That folder retains verification logs and notice/boot previews.
The GitHub release includes two ISO parts, checksums, the browser-only archive,
desktop update package/manifest, notice previews and both boot-menu captures.

The temporary build filesystem was removed after verification. Source/compile
caches, verification logs and release artifacts are retained.

## Remaining system coverage

The image retains the verified Arch package/kernel base from the previous
release; it does not claim a fresh update of all Arch packages. Full disk
installation/first-login, end-to-end authenticated release upgrades, encrypted
installation, VMware 3D, multiple displays and broader hardware still need
validation. Synaqua remains a desktop shell using KWin/Plasma services.
