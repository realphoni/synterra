# Synterra Updates

Synterra 2.0 (Variscite), Build 2010 introduces a native desktop release updater. Open **Synterra Updates** from Start or its card in Synterra Hub. It checks on opening; **Check for updates** repeats the check.

The app uses the public [GitHub Releases API](https://docs.github.com/en/rest/releases/releases#get-the-latest-release) for `realphoni/synterra`. Only published full releases are accepted. Version and build numbers are compared numerically, so older releases and reinstalling the same build do not trigger a download. This is separate from updating Arch packages.

## Download, then decide

When a compatible newer release is available, its release notes appear and its desktop update package downloads automatically. The app displays progress and checks the expected size and SHA-256 checksum before offering **Update now** or **Later**. Downloads use the normal desktop account, verified HTTPS and a per-user cache under `~/.cache/Synterra/SynterraUpdates/`. Cancel removes incomplete temporary files. A saved package is verified again when reused.

**Update now** opens Konsole with `sudo /usr/local/libexec/synterra-release-update --apply TAG PACKAGE`. The helper fetches that release and its manifest again from the fixed official repository, copies the package into a private root-owned temporary directory and independently verifies it. Symlinked/non-regular inputs, altered data, unexpected package identities, unsafe paths, privileged file modes and unrelated files are rejected.

The terminal then runs a normal full **`pacman -Syu`** transaction followed by **`pacman -U`** for the Synterra release package. Both keep pacman's confirmation prompts. Declining or failing the first transaction stops the release installation. Installing the desktop package can be retried after resolving an error. It updates distribution-owned desktop files; it does not repartition disks or copy defaults into existing user homes. Surf profiles, bookmarks, wallpaper, Glass choices and dock settings remain in their existing user locations.

Older Synterra desktop files were copied by the installer instead of being owned by a package. During the first release-package migration, the helper allows overwriting only exact approved Synterra paths that are unowned. It refuses files owned by other packages. Subsequent release packages are tracked as `synterra-release` by pacman. The branding hook refreshes `os-release`; an existing GRUB configuration gets the updated artwork while retaining disk/kernel settings.

The app never installs or reboots without your choice. Administrator authentication and package prompts happen after download. Live sessions can check release availability but do not download or apply in-place release upgrades; use the release page for an installation ISO.

## Publishing an update

Run the normal profile build, or after compiling the native applications:

```bash
bash scripts/package-update.sh /path/to/prepared/airootfs/usr/share/synterra/install-overlay /path/to/output
```

The packager uses makepkg/fakeroot to generate an ordinary x86_64 Arch package, validates its contents and writes:

- `synterra-release-2.0.2010-1-x86_64.pkg.tar.zst`
- `synterra-update.json`
- `SHA256SUMS.update`

Attach all three files to the matching **`v2.0.2010`** release. Future builds change the numbers consistently. Publish it as a full release and Latest after validation. The updater requires GitHub's `sha256:` asset digests for the manifest and package; an ISO-only release does not silently become an in-place upgrade.

Manifest schema 1:

```json
{
  "schema": 1,
  "version": "2.0",
  "build": 2010,
  "tag": "v2.0.2010",
  "architecture": "x86_64",
  "minimum_version": "1.1",
  "package": {
    "name": "synterra-release-2.0.2010-1-x86_64.pkg.tar.zst",
    "size": 123456,
    "sha256": "64 lowercase hexadecimal characters"
  }
}
```

`version` uses major.minor, `build` is a positive integer, and the tag is `vVERSION.BUILD`. The release generator supplies the actual size/hash. The minimum base version can change for a release that requires a separate migration. Build 2010's complete desktop payload supports the 1.1 base; 1.1 systems need the updater installed separately before they can use it.

Trust is anchored in HTTPS access to the official GitHub repository and GitHub's asset digests. These checks detect corrupted/replaced local downloads; they are not an independent maintainer signature and do not defend against compromise of that repository. Existing Arch repository signature policy is preserved. Release packages do not contain installer scripts or general-purpose hooks; only the existing branding hook is permitted. Standard Arch package transactions have no automatic filesystem-snapshot rollback. Review the terminal output if an update stops.

## Building and testing the app

```bash
cmake -S updater -B target/updater -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build target/updater --parallel 2
ctest --test-dir target/updater --output-on-failure
```

Qt Widgets/Network, libarchive and the usual C++ build tools are required. Tests cover numeric versions, full-release policy, manifest/asset mismatch, address restrictions, unsafe packages, real HTTP streaming, cache reuse/repair, excessive data, checksum failures, cancellation and refused redirects. Tests use a local fixture server; they do not publish dummy releases or update the host. `synterra-updates --preview --screenshot /absolute/output.png` renders the idle window without checking or installing updates.
