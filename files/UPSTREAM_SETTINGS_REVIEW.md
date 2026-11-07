# Upstream settings integration review

Date: 2026-11-05. Reviewer: settings/Euler. Coordinator: Gauss `01a1a910-fb48-7933-ac67-b1f4d5d337ae`; presentation: Noether `01a1a910-fae3-7e43-be66-3d1f3dca78fc`.

## Scope and decision

Read-only production-source comparison of `df346b5..upstream/main`, pinned to FrankieT19 `67a5f7b` (v8.0 source change `6d6084e`; the two subsequent commits concern installer deployment), against exploratory `1cca2d6`. Only this report is authored by this review. Existing unrelated carousel review/log changes were present and preserved. Only top audit checkpoints were consulted. No integration, firmware build, release, or hardware validation is claimed.

Selectively integrate wrap default On, grouped View options, Auto theme (Dark 20:00 through 07:59), and Open SYSTEM. Do not replace the current settings/launcher implementation wholesale. Upstream hardcodes clean names On, removes its reader/writer/UI, and lacks the local Remember cheats feature. Both local on/off options must survive, including persistent keys and defaults. Existing local CUSTOM Only thumbnail modes, bounded navigation, identity resolution and carousel work must survive too.

**Owner correction:** Last played truncation was observed in another/downstream build, NOT current Merkin Mix. Preserve current behavior and avoid importing a regression. Do not describe this integration as fixing a local Last played defect.

Line references below are snapshot-specific. Upstream references mean `git show 67a5f7b:source/ezkernelnew.c`; local references mean exploratory source at the reviewed snapshot. Prefer function names after integration shifts lines.

## Integration map

| Feature | Upstream transplant | Local adaptation / invariant |
|---|---|---|
| Wrap On by default | Initial value and `Launcher_ReadWrapAroundSetting` near upstream 9671; `settings_navigation.h`; menu movement call sites | Keep local canonical `Wrap list` key and fallback `Wrap around` alias at local 10152. Preserve explicit Off. Keep `Launcher_ParseOnOff` and existing browser navigation helpers; extend settings/popups without replacing browser algorithms. |
| View options | New settings IDs, nested VIEW category, separate category storage total, active-item filter, `Launcher_ViewOptionsPopup`, row text/toggle, category-window rebuilding | Retain `SETTINGS_REMEMBER_CHEATS` in Games. Retain `SETTINGS_CLEAN_LIST` in an accessible list-related location. Size visible array from item-array count if adding Clean list; upstream hardcodes seven slots. Preserve local thumbnail enum/Custom modes and retained labels. |
| Auto theme | Auto/pending state, mode name recognition/set/cycle/load/save, `settings_auto_theme.h`, clock queue and owning-screen apply/redraw sites | Persist `Theme = Auto` through the existing theme writer, not effective Light/Dark. Keep custom theme support; mode count becomes 3 or 4. No theme changes halfway through modal rendering. |
| Open SYSTEM | Root category action/request, `system_browser.h`, SET window dispatch, effective-view override, B/L/R exits | Preserve normal SD location and recents/favourites state, local safe indexed-name/path routines and custom art behavior. Port all exits together; header alone is insufficient. |
| New text | `settings_polish_text.h` eight-key supplemental lookup; updated help | Keep local main runtime enum and translations, especially Remember cheats. Do not overwrite current runtime tables with upstream versions. Canonical settings keys stay English. |
| Legacy Start settings | `Launcher_ReadStartSource`, `Launcher_ReadBootToSetting`, new Featured games wording and removal of Start enabled row | Adjacent upstream policy, not a prerequisite for the four requested features. Integrate deliberately only if desired; preserve local readers unless adopting the new policy in full. Never import the Clean list deletion migration. |
| Boot cache | `Launcher_BeginBootSettings`, reader cache branch, disable before migration write | Optional performance dependency, not required for requested settings. Prefer defer until shared scratch lifetime and equivalence are verified. |

## Defaults, persistence and migration

Local `Launcher_ReadOnOffSetting`/`launcher_setting_parse.h` accepts canonical On/Off, Yes/No, localized strings and leading 1/0, with migration for noncanonical text. Keep this infrastructure. Current Clean list, Remember cheats, List folders and Wrap default Off; Hide system files and Sounds default On.

Upstream changes missing Wrap to On and missing List folders to On. Its Wrap implementation still turns an unrecognized *present* value Off. Simply changing the local parser's default argument to 1 changes malformed-value behavior to On as well. Recommended minimal wrap change: default missing value to 1 but keep `Launcher_ParseOnOff(..., 0, ...)` for a present value, preserving current/upstream invalid-value behavior and local alias support. Document/test that distinction. Updating the initial variable alone is insufficient because boot reader resets it. List folders On is a separate upstream default change; explicitly decide whether to adopt it, rather than silently bundling it with Wrap.

Canonical key remains `Wrap list`; UI becomes localized Wrap around. Existing explicit Off must never be reset by migration. Both keys present should retain current `Wrap list` precedence. A legacy alias does not by itself trigger migration in the current reader if its value is already canonical; changing that is optional and requires a deliberate test.

Upstream `Launcher_ReadStartSource` marks any `Clean list` key for migration; its writer then drops the key and its global is `static const u16 launcher_clean_list = 1` (upstream 619). Reject all three changes. Retain mutable local state, local `Launcher_ReadCleanListSetting` call, writer at local 11381, row at 10516, toggle at 12019, and rendering guards. Default remains Off unless the owner separately asks otherwise. “Clean list names” in the requirement refers to the existing `Clean list` setting; renaming its displayed label does not require changing its persisted key.

Remember cheats needs the complete existing chain: `gl_cheat_remember`, `Launcher_ReadRememberCheatsSetting` (10147), boot read (14557), `SETTINGS_REMEMBER_CHEATS`, localized line (10533), writer (11385), toggle (12081), Games array (14096), launch guard (16903), plus existing `showcht.c` persistence. Upstream Games array omits this item. Default stays Off. Preserve disabled-state behavior and identity-checked saved selections, not merely a visible toggle.

Upstream makes Start screen always enabled, removes the old enabled row/output and interprets old `Start screen = Off` or `Start screen source = Off/2` as legacy state. It maps Boot to Start to SD for that legacy state and retains explicit other boot targets. Source choices then become Last played/Favourites under Featured games. If adopted, test all old keys and legacy files together, reader order (source before boot target), and a second boot after canonical rewrite. This migration must not consume retained Clean list or Remember cheats values.

Existing unified settings write/close handling is an acknowledged checkpoint risk (F-011), not repaired by this review. New migration paths increase exposure to that existing limitation; do not claim transactional persistence.

## Grouped View options and navigation

Upstream Interface becomes Time, Clock format, Language, Sounds, Theme, Colour, Load style, View mode, View options, Wrap around, Boot to, Featured games. It removes Hide system and Start enabled from the menu as adjacent policy changes; the Hide system reader/writer remains. Do not infer that Open SYSTEM requires deleting the Hide system toggle.

The upstream View item pool is List folders, Thumbnails, Art position, Art border, Rounded corners, Animations, Side size. Filter rules:

- List folders, Animations and Side size only in horizontal/vertical carousel.
- Art position in List+art, or carousel with unequal side size.
- Art border absent in plain List.
- Thumbnails and Rounded corners remain visible in all modes under this implementation.

Art position dispatches to vertical alignment, horizontal alignment or List+art position. `Launcher_MenuNextAlignment` presents Left/Top, Centre, Right/Bottom, Custom while preserving serialized Centre=0 and Left/Top=1. Preserve local enum reader tables and custom positions.

VIEW is a hidden nested category indexed at `SETTINGS_CATEGORY_TOTAL`; selected/top arrays must use `SETTINGS_CATEGORY_STORAGE_TOTAL`. The visible root total must NOT include the nested category. Category-window return must restore the parent active item array after opening View options. Changing Side size rebuilds filtered rows, re-finds the changed item and adjusts scroll top. Retaining a global active pointer without that rebuild risks operating on the wrong category. Tab return/resume logic must allow the hidden VIEW index and highlight Interface at the root.

`settings_navigation.h` helpers guard total 0/1, wrap only when enabled, and adjust the visible window. Port uses in actual settings/popup loops, not just the header. Keep existing local browser list/carousel navigation routines: local host scenarios already cover them, and a large upstream launcher replacement would discard local improvements. Exercise held-key repeat, wrap top/bottom repaint, popup cancel/accept and scrolling with more rows than visible.

## Auto theme lifecycle

Upstream recognizes case-insensitive Auto, computes initial dark mode from RTC hour (`UNBCD(time[0] & 0x3F)`), then cycles Light → Dark → Auto → optional Custom. Existing default remains Light. Invalid initial hour produces Light; queue ignores hour >23. Queuing records a pending effective theme (`dark + 1`, zero means none); only normal screen owners apply it and redraw. Clock updates queue changes in browser/Start/settings paths; import matching apply points and theme refresh after nested settings returns.

Test 07:59→08:00, 19:59→20:00, midnight, manual RTC adjustment, invalid hour, changing away from Auto with a pending transition, and returning from a modal opened before the boundary. Startup RTC decoding/12-hour assumptions require device validation; do not infer that the host hour predicate proves hardware RTC correctness. Preserve custom theme colours and restore all background/text/clock surfaces after the transition.

## Open SYSTEM lifecycle and risks

`system_browser.h` stores normal paths, depth, pending restoration, per-depth selection/offset arrays, and recents/favourites return state in EWRAM. Open first changes directory successfully, then snapshots the normal state, enters `/SYSTEM`, resets selection/depth and disables recents. Missing SYSTEM returns an error without installing an active snapshot. B at exactly `/SYSTEM` restores state and returns to Settings with Open SYSTEM selected. B in a child still climbs normally. L/R exits call close/restore as well.

Upstream `Launcher_UpdateEffectiveViewMode` forces List for `/SYSTEM` and descendants using a case-insensitive prefix plus boundary check; test `/SYSTEMATIC` to avoid an overly broad transplant. That override is path-based, not only action-based. Local full-name resolution, bounds and browser identity must remain intact when reading arbitrary maintenance files. Upstream also introduces `Launcher_IsGameFilename` to exclude text/Markdown from game counts; this is adjacent behavior that should be reviewed against current local filtering rather than copied incidentally.

Close restores paths and state and attempts `f_chdir(currentpath)`, with root fallback on failure. Test a removed original folder and nested/recents/favourites callers. Review all alternate exits, reentry, launch/cancel flows and SaveSDState ordering: maintenance location must not overwrite the normal saved location. Hardcoded 100-entry saved selection arrays must match local arrays. This feature adds persistent EWRAM; inspect linked map/size before shipping. No device evidence here proves the complete lifecycle.

## Text and clean-name preservation

`settings_polish_text.h` has eight labels for Open SYSTEM, Art position, View options, Auto, Featured games, Wrap around, Start screen and No Start screen. It covers English IDs E1E1/E101, European IDs E102..E10A, Chinese E2E2 and Thai 0E01, falling back to English for unknown language IDs. Chinese strings are legacy encoded bytes; Thai/European strings use UTF-8. Do not blindly re-encode the file. Its comment cites `Misc/generate_settings_polish_text.py`; that generator is not present in the reviewed changed-file set/current tracked file discovery, so do not promise regeneration without locating it.

Keep `source/launcher_runtime_text.h`, `source/launcher_text.h`, root and source `launcher_text_languages.json` consistent if changing retained labels. Local runtime enum includes Remember cheats; upstream positional table replacement could shift all subsequent strings. Supplemental lookup avoids that particular risk. Test narrow rows/long translations, Chinese and Thai fonts and On/Off values. Source text presence is not proof of translated wording quality or visual fit.

Clean list affects extension/bracket removal, right-side file details and display budgets (local uses include 1913, 2193, 2228, 3077–3268 and 3400). Preserve both raw/detail Off rendering and cleaned On rendering. Retain `Launcher_FormatIndexedDisplayName` and separation of display names from launch identities. Test bracketed names, long names sharing a clipped prefix, non-ASCII names, folders, SD/NOR and recents.

Last played has its own `Launcher_StartGetLastTitle`, splitting and scrolling pipeline (local 12880 onward), and `Launcher_PrepareLastPlayedForMenu` resolves the directory entry and builds a path with its alias. Preserve these existing paths independently of Clean list. Do not shorten launch identity to a display buffer. Coordinate Start layout/redraw changes with Noether and use current Merkin Mix as the behavioral baseline.

## Verification performed

All checks ran on the current unmodified production baseline, not an integrated candidate:

| Command | Result |
|---|---|
| `bash scripts/test-launcher-settings-readers-host.sh` | PASS: 1,275 cases identical to reference 83e350d |
| `bash scripts/test-launcher-enum-settings-host.sh` | PASS: 33,600 cases identical to reference 83e350d |
| `bash scripts/test-cheat-state-host.sh` | PASS: persistence, failures, identity, bounded selections and collapsed viewport |
| `bash scripts/test-launcher-navigation-host.sh` | PASS: launcher navigation scenarios |

The on/off differential suite intentionally compares old defaults; adopting Wrap On (or List folders On) requires explicitly scoped expected differences/new assertions. Do not weaken the entire comparison to make it green. The existing enum suite does not prove Auto theme lifecycle or new nested UI navigation. No test source was edited by this review.

## Required integration acceptance matrix

1. Fresh/no settings, missing individual keys, explicit On/Off, localized/Yes/No/numeric values, empty/invalid values, both wrap aliases: verify defaults, precedence and canonical rewrite without losing Clean list or Remember cheats.
2. Four combinations of Clean list and Remember cheats, save/reboot, unrelated setting changes and any legacy migration: both values persist and their effects remain independent.
3. Every view mode with both side sizes and every art position; enter/leave nested View repeatedly, change filtering, tab away/back, language/theme changes in nested UI; bound-check active count and category indexes.
4. Wrap On/Off at first/last item in root settings, each category, long popups, one/zero-item menus and browser views; verify repaint, selection/top, held-key repeat and cancel.
5. Auto boundaries and pending modal transitions above, Light/Dark/Custom round trips, canonical `Auto` persistence under every UI language; host hour tests plus device RTC/redraw checks.
6. Open SYSTEM from root/deep SD, recents, favourites and NOR-associated settings; missing SYSTEM, child folders, B/L/R, reentry, original-folder deletion and launch cancellation. Verify restoration of path, selection, offset, virtual mode and normal persisted SD state.
7. Current Last played short/long/multibyte display, one-line scrolling/multiline themes, favourites cycling and launch identity. Compare current baseline directly; report preservation, not a local fix.
8. Retained clean/raw filenames and details in SD/NOR/recents, clipped-prefix identity ambiguity, CUSTOM Only and style changes. Run existing long-name/identity and cheat suites after integration.
9. If boot cache is adopted: no file, empty file, exactly fitting/oversize cache, long physical lines, comments, duplicate keys, read errors, and legacy fallbacks. Cache uses `pReadCache + 0x1000` for 4096 bytes; prove scratch exclusivity through *all* local boot readers and disable before any migration/save/UI use.
10. Run appropriate host suites/full audit and forced target build on the final integrated tree; inspect EWRAM/IWRAM/ROM map and keep kernel under 2 MiB. Hardware testing remains required for RTC, input feel, restoration and visual fit.

## Risk ranking and handoff

High: destructive loss of retained settings through upstream writer/migration/global replacement; dropping local Games/translation entries; wrong nested-category storage or filtered-array capacity; incomplete SYSTEM state restoration; reverting safe names/navigation through a broad source replacement.

Medium: present-invalid vs missing Wrap semantics; incidental List folders/Start/Hide-system policy changes; stale active-category pointers; mixed theme redraw across modals; localization width/encoding; optional boot-cache scratch lifetime and migration timing.

Recommended order: retain settings/readers and add explicit default tests; add supplemental text and Auto state; integrate grouped navigation with retained options; integrate SYSTEM lifecycle and restoration tests; coordinate presentation changes; validate final candidate. Keep optional cache and unrelated upstream behavior changes separate. No production edit is authorized by this review assignment.

## Follow-up: current transplant lifecycle review

Reviewed the coordinator's working-tree transplant after the initial report (still read-only source review). Coordinator retains Start enable, Hide system, Clean list and Remember cheats; omits the boot cache; keeps List folders default unchanged. `system_browser.h` now uses `MAX_path_len` selection/offset arrays, matching current production arrays. Findings below refer to function names and approximate working-tree lines; they are findings at inspection time, not assertions that later coordinator edits remain defective.

### Confirmed Auto repaint gaps

1. **Settings root queues but never applies.** `Launcher_SettingsWindow` main loop (around 14567) calls `Launcher_DrawSettingsClock(0)` but has no `Launcher_ApplyPendingAutoTheme`. Leave the root Settings page open across 08:00 or 20:00: the pending change remains unapplied until another owner handles it. Add the normal owner apply/dirty path here, as in the category/help loops. `Launcher_SettingsDrawList` already performs full background/topbar/forced-clock redraw when dirty.
2. **Start preserved-underlay optimization can suppress a required theme repaint.** The Start loop applies the pending theme, settles motion and sets dirty, but does not clear `launcher_start_window_preserved`. Last played modal cancellation can set that flag before returning to Start (around 15304). `Launcher_DrawStartWindow` immediately clears the flag and returns without drawing (around 13992). If Auto changes at that return, the old underlay remains while colours/state now describe the new theme; subsequent updates can mix palettes. Invalidate the preserved-underlay flag on an effective Auto change before drawing Start. Reproduce with Auto at 19:59, open Last played menu, cross 20:00, cancel. This is an integration repaint issue, **not** the owner's unrelated downstream title-truncation regression.

Category and Help topic-list owners do apply and fully redraw. Browser SD/NOR applies pending changes at the outer normal-loop boundary and sets force-full-redraw, system-name and clock dirty before `re_showfile`. Start's ordinary unpreserved redraw includes a forced clock. Modal loops queue through clock updates without directly applying, consistent with the intended ownership rule. This static inspection does not prove all visual combinations on device.

### Confirmed SYSTEM alternate-exit gap

3. **SD menu return-to-NOR bypasses maintenance close.** In the main SD file action branch (around 16341), `SD_list_MENU(...)` returning 2 sets `page_num = NOR_list` without `Launcher_CloseSystemBrowser`. The menu has return-2 paths after loading to NOR. Reproduction: Open SYSTEM, select a launchable GBA beneath SYSTEM, load it to NOR, then return NOR→SD. The active maintenance snapshot survives and the saved/current maintenance path can be restored instead of the original normal SD location. Add close/restore when the menu causes this page transition, before NOR state takes over; audit the other `res == 2`/`menu_res == 2` transitions for the same invariant. A cancelled menu that stays in SYSTEM must retain the maintenance session.

### Paths inspected without a new confirmed blocker

- Direct B at `/SYSTEM` restores the snapshot and returns Settings; child B walks up normally. Direct L/R call close after saving the maintenance cursor: close restores the original saved path/arrays, so this ordering by itself does not lose the normal snapshot.
- Opening SYSTEM changes directory before marking/saving a new session; missing folder returns without installing the snapshot. Reentry while active preserves the original snapshot.
- `Launcher_SaveSDState` only updates in-memory path/depth; it is not a settings-file write. Temporary calls during delete/favourite prompts are undone by eventual successful close.
- Short START can open recents while maintenance is active; B returns to the maintenance path, while L/R switch recents/favourites. This is not inherently a leaked exit, but needs explicit acceptance coverage alongside the NOR transition above.
- The close fallback for a missing original path changes to root and disables recents. Existing selection clamping occurs on subsequent SD rebuild. Device/fault-path tests remain necessary; this review did not simulate removed directories.

No new test/build claim is made for the changing transplant. The earlier four PASS results remain baseline-only. All three concrete findings were sent directly to Gauss; the Auto/Start findings were also sent to Noether. No production source or test source was edited by this reviewer.
