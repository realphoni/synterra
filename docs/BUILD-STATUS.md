# Build status — 2026-10-02

Synterra Indev (Prism 0.4) built successfully with Archiso in `SynterraBuild`.

- Image: `synterra-0.4.20261002-x86_64.iso`.
- Size: 2,627,321,856 bytes.
- SHA-256: `6bd6316c11cc20acfabff072ac97aab849e459af293687e07e6eec8d7fa8d0a0`.
- Prepared assets: two ISO parts, whole-image and part checksums, plus a separate Surf update archive and checksum.

## Prism 0.4

Surf now includes private windows, searchable local history, find-in-page, reopen-closed-tab, optional session restoration, favicons, dark-toolbar settings and a download manager with progress/cancel/folder controls. Address parsing supports ports with paths and IPv6. Session restoration is off by default. History keeps 200 recent pages, sessions at most 30 tabs, and saved URLs strip credentials. Private profiles do not save Surf history or sessions; explicit bookmarks and downloads still persist.

Okular, VLC with its media plugins, and Filelight join the existing desktop applications. Default PDF and common media associations are included. Surf is available in both the live and installed-system payloads, and a separate browser archive supports updating existing Synterra installations.

## Validation

Native browser compilation and source/installer regression checks passed. Tests verify URL normalization, history deduplication/cap, credential stripping, saved-settings persistence, allowed session URLs and private isolation. A sandboxed browser test under an unprivileged user rendered the start page, exercised Ctrl+F and actual find matches, checked tab closure and an off-the-record profile, and downloaded a local HTTP fixture with correct contents and completion state. Its screenshot was visually inspected.

The new packages were confirmed in the staged ISO filesystem, where Surf's self-tests also passed. Archiso completed package installation, initramfs, SquashFS and ISO creation. Both live and installer-payload Surf executables have executable permissions inside the compressed image. The original wallpapers remain unchanged.

This ISO has not been boot-tested or installed to a disposable VM disk. Broad website compatibility, download cancellation/close confirmation, settings/history dialogs and desktop file associations still need manual testing. Release signing remains future work.

The installer retains the tested COSMIC-to-SDDM repair and guarded recovery menu entry. Choose Exit after Archinstall completes and wait for Synterra finalization before rebooting.
