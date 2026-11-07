# DS Style Merkin Mix

A custom kernel for the **EZ-FLASH OMEGA Definitive Edition**, maintained by Merkin ([voluptuary](https://github.com/voluptuary)). This fork builds on [FrankieT19's DS Style](https://github.com/FrankieT19/omega-de-ds-style-kernel), Sterophonick's SimpleLight, and the official EZ-FLASH kernel.

> [!WARNING]
> **Definitive Edition only.** Do not install this kernel on the original EZ-FLASH OMEGA.

## Downloads and status

Download `ezkernelnew.bin` from [Merkin Mix releases](https://github.com/voluptuary/ezfode-dssmm-kernel/releases). Copy it to the SD-card root, then boot the cartridge while holding **R** to update the kernel. Back up saves before updating.

## What Merkin Mix changes

- **8.3.0:** Refined startup, menu feedback and Start-screen motion; added Auto theme, grouped artwork settings, Open SYSTEM, default wrap-around with saved preferences preserved, and Boot to. Clean list and Remember cheats remain optional. Thumbnail scale maps now reside in IWRAM.
- Mirrored SD-card artwork lookup for multi-system libraries, with custom and built-in thumbnail fallbacks.
- Selective transparency in marked thumbnails, including support in animated carousel frames. Enabled artwork borders use rectangular or rounded outlines.
- Optional **Settings → Games → Remember cheats**, off by default, with localized labels (Thai uses English). Selections save when leaving the cheat editor and reload for the same game, including launch without reopening the editor. Collapsing a group preserves its selections, clamps the cursor and clears stale rows.
- Refreshed startup artwork and system icons, plus Merkin Mix branding.
- Optional browser wrapping; the v8.2.0 source accepts both upstream's `Wrap list` and the earlier fork's `Wrap around` setting.
- A `CUSTOM Only` rounded-corner mode in v8.2.0 for artwork loaded from `/SYSTEM/IMGS/CUSTOM` or `/SYSTEM/IMGS2/CUSTOM`. Art borders remain independently configurable. This mode has owner-reported hardware confirmation on the prior candidate; the final rollback build has not been hardware-tested.
- Long-filename/path fixes, defensive save and SD I/O checks, bounded cheat parsing, NOR-operation checks, and rejection of oversized fragmented-file tables.
- A build-time 2 MiB kernel-size limit, plus carousel alignment and wrap-transition fixes in v8.2.0.

The **inherited DS Style interface** includes list, List + Art, horizontal and vertical carousel views; title and box artwork; themes; favourites and recent games; multilingual text; UI sounds; a text/Markdown viewer; and readable `/SYSTEM/SETTINGS.TXT` settings. Smooth carousel animation, adjustable side previews, and the launch pulse come from **FrankieT19's v7.5**, integrated into the 8.2.0 source.

Host regressions and builds do not establish hardware compatibility or timing. Open audit items include failed overwrites that can lose an existing SD save, settings-file write/close failures, NOR catalog recovery, and NOR operation timing. See the [audit progress ledger](progress.md) for evidence and remaining limits.

Remembered selections use `/SYSTEM/CHEATSTATE/<hash>.cs2`, with full game-path identity and a cheat-content fingerprint. Changed cheat files invalidate the snapshot; legacy `CHS1`/`.cst` files are not imported. The snapshot supports up to 128 selected options, one per section; persistence is disabled for cheat files larger than 1 MiB. Failed writes are reported, but replacement is not atomic and can lose the prior snapshot.

## Artwork and documentation

[DS Style Card Companion](https://github.com/voluptuary/dsscc) is the separate artwork/scraper companion fork.

Transparent thumbnails must retain the `DST1` marker in BMP header bytes 6–9. The supported layout is an uncompressed, top-down 16-bit BMP: **120 × 80** for title artwork or **80 × 80** for box artwork. In marked images, RGB555 `0x0000` is transparent; use nonzero near-black for visible black. This is selective transparency, not a full alpha channel. Unmarked thumbnails remain opaque.

The [upstream user guide](https://frankiet19.github.io/omega-de-ds-style-kernel/) covers everyday use, artwork and customisation. The [upstream Manager](https://frankiet19.github.io/omega-de-ds-style-kernel/manager/) and Customiser belong to DS Style; their kernels and style packages should not be assumed to contain Merkin Mix changes.

## Build from source

Use devkitPro with **devkitARM, GBA rules, libgba, Maxmod and Make**. The checked-in devcontainer uses `devkitpro/devkitarm:20260610`; rebuild the container when opening a fresh Codespace. The recorded audit toolchain is devkitARM 16.1.0.

From the repository root, with the devkitPro environment loaded:

```sh
make -B && cp "$(basename "$PWD").gba" ezkernelnew.bin
```

`make` names its `.gba` output after the checkout directory. The `.bin` is a copy of that raw image. The Makefile rejects images over **2,097,152 bytes**; do not bypass this limit. Only copy an image after a successful build.

On Windows, install devkitPro under `C:\devkitPro` and use its MSYS environment for the command above. The included `build.bat` sets those fixed paths, but its copy step expects `x.gba` (or a temporary drive-letter output for paths containing spaces). In an ordinary checkout, verify the actual directory-named `.gba` and copy it yourself; an existing `ezkernelnew.bin` is not proof that the wrapper produced a fresh build.

Generated theme headers are checked in. If you change theme images, regenerate them with `Grit\Build Skin Files.bat` before building.

For the broader build and regression checks, with the audit dependencies installed:

```sh
bash scripts/rebuild-and-verify-audit.sh
```

The script also uses host test tools, Node and Cppcheck; consult the script for its checks. It does not publish a release or refresh the candidate directory automatically.

## Credits and license

Thanks to **Sterophonick**, **FrankieT19**, **exploitz86**, and **SkillerCMP** for DS Style and the related kernel contributions on which this work draws. See [CREDITS.md](CREDITS.md) for provenance and the distinction between inherited features and the 8.2.0 integration.

The repository includes the [Apache License 2.0](LICENSE). Retain the individual component licenses and attribution notices.
