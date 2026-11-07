# Carousel exploratory candidate — 8.2.0-exp1

The owner reports **gradual slowdown as folder size increases**, becoming noticeable around 60 titles. There is no claimed hard threshold. This candidate reduces bounded CUSTOM manifest lookup work; it does **not** establish a fix for overall folder-size-dependent scrolling latency. Hardware comparison remains necessary.

## Candidate identity

- Source implementation checkpoint: `1cca2d63568faa72fa728c35ebeec3f36ceeeaba` on `exploratory`; subsequent validation commits change evidence/documentation/artifact only.
- Candidate: [`candidate/8.2.0-exp1/ezkernelnew.bin`](candidate/8.2.0-exp1/ezkernelnew.bin).
- Size: **2,064,384 bytes**, **32,768 below 2 MiB**, +104 bytes versus stable.
- SHA-256: **`7bb7cb533b8ac7ce88a2893a1c7665d9c4d36e78acad33a14528410c4363e555`**.
- About: **`DS Style Merkin Mix 8.2.0-exp1`**.
- Stable v8.2.0 remains [`candidate/8.2.0/ezkernelnew.bin`](candidate/8.2.0/ezkernelnew.bin), 2,064,280 bytes, SHA-256 `72201320f0e8fc7c332a715704fc11a8c1ad33e8400661d09bb45c411ac3d8fa`, release source `9211e71bd435a14bdfac1ee5af57be4675d8a719`. No tag, release or PR is created for this experiment. Owner README changes are preserved.

## Change and measured scope

After collecting the existing per-style CUSTOM manifest, an iterative in-place Shell sort orders its populated hashes. Binary search replaces linear hash membership. The first 256 collected hashes remain the same set, including duplicates; a matching hash still requires the original exact filename open. Artwork precedence, CUSTOM Only provenance, long-name resolution, caches, rendering and shared-buffer ownership are unchanged. Sorting only happens during the existing initial manifest load, including a partially collected directory on read failure; missing-directory and warm reuse behavior are unchanged.

`test-custom-manifest-host.sh` extracts the actual production sort, lookup, collection and gate functions. It compares against linear membership for **169,620 queries**, counts 0..256, ordered/reversed/duplicate/small-domain/full-width hashes, checks guard canaries, and tests first-256 selection, style separation, invalid-style fallback, partial reads, empty/missing directories and warm reuse. I/O/hash/normalization are deterministic host stubs; this is not a full artwork pipeline emulation.

| Manifest hashes M | Old probes for 120 misses | New probes for 120 misses | One-time reverse-order sort comparisons |
| --- | ---: | ---: | ---: |
| 30 | 3,600 | 480 | 138 |
| 60 | 7,200 | 600 | 357 |
| 90 | 10,800 | 720 | 607 |
| 120 | 14,400 | 720 | 885 |
| 256 | 30,720 | 960 | 2,562 |

These are hash probes, not instruction counts. Binary search can make two relational/equality checks per probe; early linear hits can be cheaper. Sorting adds cold CPU work, including writes not counted in the comparison column. Reverse-order sort results are a fixture, not a worst-case bound or measured target time. CUSTOM size is independent of active ROM-folder size. No expected percentage hardware speedup is asserted.

`test-carousel-lookup-scaling-host.sh` instruments only a temporary host copy of real FatFs `dir_find`, `dir_next`, `f_open` and `f_read`, reusing the RAM-disk long-name fixture. N ordinary LFNs plus one clipped LFN are created per directory. One late ordinary LFN open traverses 121/242/363/484/965 `dir_next` calls at N=30/60/90/120/240; a missing name traverses 132/253/374/495/976. Each case has one open and two `dir_find` calls. Directory-slot and parent traversal counts are not disk-sector reads. Parent directories accumulate in the fixture, accounting for a small part of the increase. Ordinary name copies use zero explicit resolver calls; clipped resolution returns N+1 entries. The explicit resolver callback is the existing test model, while the FatFs open implementation is production code.

This establishes lookup scaling, not an end-to-end per-move benchmark. The reviews in [CAROUSEL_PERFORMANCE_REVIEW.md](CAROUSEL_PERFORMANCE_REVIEW.md) and [CAROUSEL_RENDER_REVIEW.md](CAROUSEL_RENDER_REVIEW.md) trace the fixed three-slot cache and three/four-image render work. Remaining costs include ambiguity-checked clipped-name scans, FAT directory searches, ROM-header fallback and missing-artwork opens. No new negative cache or resolver early exit was introduced because either would expand identity/invalidation semantics beyond this narrow change.

## Target footprint and validation

Forced `make -B` passed with devkitARM 16.1.0. Candidate bytes match the resulting `.gba`. The new routines inline into the existing loader/gate. [Target disassembly](carousel-performance/manifest-arm-disassembly.txt) shows unsigned comparisons, shifts for division by two, word-indexed loads/stores, no allocator, sort-library dependency or recursion. Manifest loader is 484 bytes (baseline 408). Its local frame is 32 bytes including saved registers; the CUSTOM file loader/gate frame is 48 bytes including saved registers. These are static local frame observations, not runtime peak-stack measurements.

Linked thumbnail workspace stays at `0x02000bfc`, size `0x7e90`, word-aligned. EWRAM `__end__` stays `0x020383d0`; no persistent RAM is added. Motion code is `0x08026880..0x0802736c`, size `0xaec` (2,796), still within the `0x3e00` relocation window. No timer, interrupt, VRAM layout, DMA or audio policy changes.

Full validation: PASS. All 19 host suites and the full audit completed successfully; durable logs below include the final completion marker. Existing cheat-row host compiler warnings remain. No hardware validation was performed.

Durable logs: [forced build](carousel-performance/build.log), [focused manifest](carousel-performance/focused-manifest.log), [focused scaling](carousel-performance/focused-scaling.log), [all host suites](carousel-performance/all-host-suites.log), [full audit](carousel-performance/full-audit.log). Reproduce from the repository root:

```sh
bash scripts/test-custom-manifest-host.sh
bash scripts/test-carousel-lookup-scaling-host.sh
make -B
for suite in scripts/test-*-host.sh; do bash "$suite" || exit; done
bash scripts/rebuild-and-verify-audit.sh
sha256sum files/candidate/8.2.0-exp1/ezkernelnew.bin
cmp omega-de-ds-style-multisystem-kernel.gba files/candidate/8.2.0-exp1/ezkernelnew.bin
```

The generated `.gba` name follows the checkout directory name. A differently named checkout should substitute its generated filename. No host pass establishes SD timing, hardware compatibility or overall responsiveness. Existing audit risks F-011/F-022/F-032/F-036 remain open. Historical CUSTOM 256-entry cap/hash-collision behavior is preserved, not fixed.

## Hardware comparison and next action

Use the normal kernel update workflow to compare the separately staged `ezkernelnew.bin` with the preserved v8.2.0 baseline; verify About before each run. Use the same console, SD card, settings and slow folder. Record initial folder entry separately from the first move, 20 forward single steps, held scrolling, reverse oscillation and both wrap boundaries. Repeat after a restart to distinguish the initial per-style manifest load from warm lookup. Do not infer improvement solely from a differently populated/copied SD card.

1. Start with the owner's actual slow folder and identical art/settings. Report orientation, animations, title/box art, approximate ROM and CUSTOM counts, filename lengths and whether artwork is missing. Record whether the delay is before motion begins or during motion.
2. Compare 30/60/90/120/240-title fixtures holding image bytes/dimensions and CUSTOM directory size constant. Include short names and >=99-byte names sharing display prefixes with distinct full names/art. Keep physical file order/placement comparable and record it; lexical order alone does not determine FAT traversal.
3. Separately vary CUSTOM count with ROM count fixed, including no CUSTOM directory and 256 entries. Check initial loading for the added sort cost; then compare warm scrolling. The manifest optimization is most exposed on repeated membership misses, and benefits may be too small to notice beside SD lookup.
4. Cover horizontal/vertical, animations on/off, title/box, opaque/transparent/missing artwork, folders, canonical/alias mirrored art, CUSTOM named art, game-code/default fallback, CUSTOM Only corners, and 1/2/3-entry wraps. Verify folder/style/view changes, recents and returning from menus do not show wrong or stale artwork.

If latency persists, next isolate which synchronous open/search dominates the real folder before adding another cache. Extend the host harness to extract the real thumbnail-load path and count per-move opens/rebuilds/incoming reuse across this matrix. A bounded exact-identity name/game-code cache or definite-absence extension cache may then be evaluated with explicit generation/media/error invalidation and a RAM budget. Avoid whole-directory indexing or timing instrumentation without ownership analysis. No claim that this candidate resolves the reported gradual slowdown is warranted yet.
