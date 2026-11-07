# DS Style Merkin Mix 8.3.0-rc2 — exploratory test candidate

## Candidate identity

- Candidate: [`candidate/8.3.0-rc2/ezkernelnew.bin`](candidate/8.3.0-rc2/ezkernelnew.bin)
- Size: **2,009,936 bytes**, **87,216 bytes below the 2 MiB limit**
- SHA-256: **`b02cf64b0862ac0b75bbb8b26ceca099a5f60cc2850b03bdcb2073e916934705`**
- About version: **DS Style Merkin Mix 8.3.0-rc2**
- Stable baseline: v8.2.0, 2,064,280 bytes, SHA-256 `72201320f0e8fc7c332a715704fc11a8c1ad33e8400661d09bb45c411ac3d8fa`

## Changes since 8.3.0-rc1

- Saving Game Data now displays the matching 80x80 box artwork from the box-art artwork set instead of title artwork. Artwork remains optional; invalid or missing images do not prevent save recovery.
- Wrap around includes **On, Not SYSTEM**, which disables wrap-around under `/SYSTEM` and its subdirectories while preserving normal SD, Recents, and NOR behavior.
- Clean list Off now invalidates cached rows and redraws immediately.

## Validation and limits

- Forced production build with devkitARM 16.1.0: PASS; 2,009,936 bytes.
- All host regression scripts: PASS, including save-popup artwork dimensions, SYSTEM-scoped wrap-around, Clean list cache invalidation and saved settings parsing.
- `scripts/rebuild-and-verify-audit.sh`: PASS, including Cppcheck.
- No emulator or physical-hardware validation was performed for this candidate.

This is an exploratory prerelease, not a stable release. The v8.2.0 stable baseline remains unchanged.
