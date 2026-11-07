# DS Style Merkin Mix 8.3.0-rc3 — exploratory test candidate

## Candidate identity

- Candidate: [`candidate/8.3.0-rc3/ezkernelnew.bin`](candidate/8.3.0-rc3/ezkernelnew.bin)
- Size: **2,009,824 bytes**, **87,328 bytes below the 2 MiB limit**
- SHA-256: **`e27a53d4ec85a3d0c268177b67a7fdf6aefa81191dad6c7e0003e02c982b9bf4`**
- About version: **DS Style Merkin Mix 8.3.0-rc3**
- Stable baseline: v8.2.0, 2,064,280 bytes, SHA-256 `72201320f0e8fc7c332a715704fc11a8c1ad33e8400661d09bb45c411ac3d8fa`

## Changes since 8.3.0-rc2

- Clean list Off preserves filename extensions and bracketed text in horizontal and vertical browser titles; On continues to show cleaned titles.
- Reflowed the “Using DS Style” help content into 13 nine-line pages, with each section heading at the top and up to eight description lines.
- Aligned LED setting values by widening the label column one character.

## Validation and limits

- Forced production build with devkitARM 16.1.0: PASS; 2,009,824 bytes.
- `scripts/rebuild-and-verify-audit.sh`: PASS, including host regressions and Cppcheck.
- No emulator or physical-hardware validation was performed for this candidate.

This is an exploratory prerelease, not a stable release. The v8.2.0 stable baseline remains unchanged.
