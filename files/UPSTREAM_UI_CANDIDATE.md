# Upstream UI integration — 8.3.0-rc1

Exploratory test candidate on `exploratory`; not a stable release. The v8.2.0 tag and its candidate remain unchanged.

## Candidate identity

- Candidate: [`candidate/8.3.0-rc1/ezkernelnew.bin`](candidate/8.3.0-rc1/ezkernelnew.bin)
- Size: **2,009,752 bytes**, **87,400 bytes below the 2 MiB limit**
- SHA-256: **`4dfe5f0c1bfc4bda1b58f71c6b03fab0438e612a31725f9c3b516afb2033476f`**
- About version: **DS Style Merkin Mix 8.3.0-rc1**
- Stable baseline: v8.2.0, 2,064,280 bytes, SHA-256 `72201320f0e8fc7c332a715704fc11a8c1ad33e8400661d09bb45c411ac3d8fa`

## Included behavior

The existing default-on horizontal and vertical carousel motion, side-preview sizing choices and direct-launch selection pulse are retained from v8.2.0. This candidate refines Start-screen corner-marker easing, menu sounds and pop-ups, launch loading-ring placement and theme-matched fade, startup presentation, and title artwork in the Saving Game Data display.

Settings add clock-based Auto theme (dark 20:00–08:00), grouped View artwork controls, and Open SYSTEM. Wrap-around browsing and menu navigation default On; explicitly saved Off values still load Off. Double-tap Select immediately toggles a favourite from the SD list, Recents or Favourites without a confirmation dialog. Boot to selects the startup destination; the Start screen remains available. Clean list and Remember cheats remain user-configurable, including their Off values.

## Validation and limits

- Forced production build with devkitARM 16.1.0: PASS; 2,009,752 bytes.
- Host regressions: all test scripts pass, including the focused favourite add/remove, capacity and blocked-cache cases; settings reader tests verify saved Off values and the default-On wrap preference.
- Full audit: PASS. Production build, artwork/CUSTOM Only, carousel bounds, border rollback, cheat state, browser installer, JavaScript syntax, whitespace and Cppcheck.
- Link map: `pReadCache` at `0x02013ed0`, size `0x20000`; EWRAM `__end__` at `0x0203a140`, below the configured heap end `0x0203c000`. New EWRAM allocations include the SYSTEM browser snapshot (`0x0b2c` bytes), motion state (`0x05a4`) and launch indicator snapshot (`0x0ca4`). The shared audio buffer remains separate; launch artwork scratch is copied to OBJ VRAM before loader work reuses it.
- No emulator or physical-hardware validation was performed for this candidate. Hardware checks remain necessary for presentation, audio/IRQ ownership, startup, save art and launch teardown.

No GitHub release or stable tag is created by this candidate.
