---
phase: 04-vce-vdc-raw-video
plan: 12
subsystem: vdc-scroll-latch-and-enables
tags: [vid, scroll, latch, bxr, byr, wrap, burst, backdrop, bb-sb, write-join, margin-guard, flat-checks, fault-sweep, toy, chunk-equiv, snapshot-restore, bg-scroll, bg-enab, w5]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the background renderer, the BYR counter bg_y and its one-statement BYR write path (04-11), the video judge with the frame selector and the sweep machinery (04-05, 04-07, 04-11), the dev sink and the stamps of the accesses (04-02, 04-19)
provides:
  - tests/conformance/videokinds.c - the write join of the latch rows (the dev scan records BXR, BYR and VDC CR data writes; a selector line=bxr N+D, byr N+D or cr N+D finds line n as the number of record origins before the stamp, less one), the 250-clock margin guard as a fault of the case, the REL
    at= form (scroll_x_shift on the two lines around a write, scroll_y_shift against the frame before) and the STRUCT checks flat_frame and flat_at, with 19 judge-forms rows
  - scripts/lib/vexp-rules.awk - the grammar of those selectors and checks (the closed lists)
  - fixtures/src/bg_scroll.s (id 23) with its checks, vexp and manifest - counted mid-frame BXR 16 / 0 and BYR 63 / 0 writes on a 32 x 32 BAT of period-4 solid tiles, then a 64 x 64 BAT with SCREEN 101, BXR 500 and BYR 300
  - fixtures/src/bg_enab.s (id 24) with its checks, vexp and manifest - the VDC CR written with BB and SB in every combination (burst frame, backdrop) and a BB clear inside a display line
  - scenes video_bg_scroll (MAN H7-7 next-line rows for BXR and BYR, ASSUMED own-line and counter rows, REL rows), video_bg_scroll_asym (direction and wrap, ASSUMED LED-VID-0010) and video_bg_enab; three fault sweeps, two mutation sweeps, chunk_equiv_video_bg_scroll and snapshot_restore_video_bg
  - tests/controls/vid-scroll-latch - the toy, defined as "the BYR write no longer reloads the row counter" (see the finding below)
affects: [04-13, 04-14, 04-15, 04-16]

requirements-completed: [VID-01]
requirements-advanced: [VID-06]

actuals:
  tokens: 24300
  tasks: 3
  commits: 3
plan_head_before: 607ad49318574eb21699fe41ab9f2355a57c25f6
plan_head_after: f92f1625fd5a941113a474cabbac0ed2e0fd238c

tech-stack:
  added: []
  patterns:
    - "A latch row names a register write of the dev log, not a line number: the line is the record whose origin is the last one before the write stamp (a count of origins, so a fault that swaps or duplicates records moves what the row reads), and a write within 250 master clocks of any origin is a
      harness_failure with the offsets printed, so the counted delay can be placed and the tie rule never decides a row"
    - "Direction and wrap independence by construction (ER-12): a scroll of two tiles over a row whose period is four tiles gives the same line whichever way the scroll runs, so the next-line rows are MAN; the direction and the 64 x 64 wrap are a separate ASSUMED scene"
    - "A flat line set (a burst frame, the backdrop) cannot be a ROWS row (D-171 non-vacuity), so it is a STRUCT check that also carries the BURST flag; the sweep keeps it live with F0, F3 and F8"
    - "A fault only kills a group when the line it exchanges differs: a group whose first row sits on the first display line, or on a line whose neighbour has the same tile row, cannot declare F8"

key-files:
  created:
    - fixtures/src/bg_scroll.s
    - fixtures/src/bg_scroll.checks
    - fixtures/src/bg_scroll.vexp
    - fixtures/manifest/bg_scroll.toml
    - fixtures/src/bg_enab.s
    - fixtures/src/bg_enab.checks
    - fixtures/src/bg_enab.vexp
    - fixtures/manifest/bg_enab.toml
    - tests/conformance/cases/bg_scroll.toml
    - tests/conformance/cases/mutate_sweep_bg_scroll.toml
    - tests/conformance/cases/video_bg_scroll.toml
    - tests/conformance/cases/video_bg_scroll_asym.toml
    - tests/conformance/cases/video_fault_sweep_bg_scroll.toml
    - tests/conformance/cases/video_fault_sweep_bg_scroll_asym.toml
    - tests/conformance/cases/bg_enab.toml
    - tests/conformance/cases/mutate_sweep_bg_enab.toml
    - tests/conformance/cases/video_bg_enab.toml
    - tests/conformance/cases/video_fault_sweep_bg_enab.toml
    - tests/conformance/cases/chunk_equiv_video_bg_scroll.toml
    - tests/conformance/cases/snapshot_restore_video_bg.toml
    - tests/controls/vid-scroll-latch/RULE
    - tests/controls/vid-scroll-latch/EXPECT
  modified:
    - tests/conformance/videokinds.c
    - tests/conformance/obskinds.c
    - scripts/lib/vexp-rules.awk
    - tests/conformance/REQUIRED
    - tests/conformance/VIDEO_TOY_EXEMPT
    - ledger/entries/LED-VID-0009.toml
    - ledger/entries/LED-VID-0010.toml
    - .planning/phases/04-vce-vdc-raw-video/04-VALIDATION.md

key-decisions:
  - "The BXR latch rows use BXR = 16 (two tiles), not 8 as the plan text says: the rows must be MAN (the page states the next-line rule, not the direction), and a one-tile shift of a period-2 row would leave only two distinct values, which the non-vacuity rule of D-171 refuses. A shift of 16 dots
    over a row of period 32 dots gives the same line in either direction and over the 256-dot wrap"
  - "The bank of a BAT row is 1 for rows 0 to 7 and (r & 3) + 2 from row 8, so the literals of the latch rows do not depend on the exact display line of a write (all four writes fall in lines 16 to 72, calibrated from the counted sequence) and a BYR write of 63 shows row 64, the BAT row 8 and bank 2, where the old counter or the written value itself would show bank 1"
  - "A latch line is found by counting origins before the stamp, not by comparing a stamp with a grid: no 1365 constant enters the join, and the margin guard is the only place the tie rule could matter"
  - "The burst frame and the backdrop are classed ASSUMED LED-VID-0010, not MAN H7-6 as the plan text says: the page says SB = BB = 0 starts burst mode with the next frame, and what a burst frame shows (index 256) and the latch instant at display line 0 are the withheld words burst timing and backdrop colour of that entry (ER-10, the conservative class)"

completed: 2026-11-12
---

# Phase 4 Plan 12: Scroll latch, enables and burst Summary

**A BXR or BYR write at a counted instant inside a display line is located by its dev-log stamp against the record origins and shown on the next line and not its own (MAN H7-7), the 64 x 64 wrap and the scroll direction are judged in a separate ASSUMED scene, the BB/SB matrix, the burst frame, the
backdrop index and a BB clear written mid-frame are judged by hand-derived rows, the scroll scene is chunk- and restore-invariant, and the toy vid-scroll-latch turns exactly the BYR rows red.**

## What was built

- **Task 1, tracer (commit 864f7f3).** The judge learned the write join: `vk_scan_dev` records the data writes of BXR, BYR and the VDC CR, `vk_wjoin` finds line n (the number of origins before the low-byte stamp, less one), refuses the row as a fault of the case when any origin is within 250 master
  clocks of either data write stamp (printing both offsets) or when the two data writes fall in two lines, and a ROWS selector `line=bxr1+1;x=0..31` names the line D after it. A REL selector may end in `;at=bxr1`. The fixture `bg_scroll` (id 23) and the scene `video_bg_scroll` (struct rows, the BXR
  rows `latch` MAN H7-7, the REL rows `latch_rel`) were written before any DuoSpool output was read; the first run mismatched only because the write fell in a later display line than the literal assumed (the bank is row dependent), so the delay was recalibrated. `obskinds.c` OX_ROMS_MAX 32 to 40
  (the 33rd guest ROM).
- **Task 2 (commit 7498e5e).** The BYR rows (`byr_next` MAN H7-7: line n + 1 shows row BYR + 1, so 63 shows the BAT row 8 of bank 2; `byr_own` ASSUMED LED-VID-0009 for the lines that hold a write and the line after the next; `byr_rel` REL scroll_y_shift against the frame before), the wrap scene
  `video_bg_scroll_asym` (BXR 500, BYR 300, a 64 x 64 BAT: direction, the column wrap at dot 12 and the row wrap at display lines 211 and 212, all ASSUMED LED-VID-0010), the fixture `bg_enab` (id 24) and `video_bg_enab` (bb_on MAN H7-6, burst and backdrop via the new STRUCT checks flat_frame, flags
  via frame_flags, toggle_own MAN H7-6, toggle_next flat_at ASSUMED), three fault sweeps (every declared fault flips: scroll 20 of 20, asym 8 of 8, enab all) and two mutation sweeps.
- **Task 3 (commit f92f162).** `tests/controls/vid-scroll-latch` (anchor: the one statement of the BYR write path in vdc.c, replacement an empty statement; EXPECT `video_bg_scroll` and `video_fault_sweep_bg_scroll`), `chunk_equiv_video_bg_scroll` (chunks 7, 1364, 1366, 3072, vframe, 4,000,000
  clocks), `snapshot_restore_video_bg` (the 1364 and 1366 schedules at every stop and the 7 schedule at every 2003rd stop and every line event, three policies, 1,560,000 clocks, which holds the four counted writes), the ledger pins and the A3 and A4 rows of 04-VALIDATION.md.

## Finding recorded: the toy of D-173 as worded cannot be built (plan-check ruling 4)

D-173 words the toy as "scroll applied to the current line". Under R1 the record of the line that holds a write is emitted at its own line event, before the write, so a renderer that applied a scroll "to the current line" would show the same records as the real one from line n + 1 on, for BXR and
for BYR. The toy is therefore the latch rule of D-156 removed: the BYR write no longer reloads the row counter (`inst->bg_y = ...` in vdc.c replaced by an empty statement), which the next-line literal kills. Observed: a run of every case against the toy binary turns exactly `video_bg_scroll` and
`video_fault_sweep_bg_scroll` guest_failure (the other non-pass results of that run are harness failures of cases that need input files the ad hoc run does not give them, unrelated to the toy). The redefinition is in 04-VALIDATION.md A3.

The BXR rows have no renderer mutant (toy cap spent). Liveness evidence recorded in A4: sweep F1 and F8 flip the group `latch` (declared F0, F1, F7, F8) and the REL group `latch_rel` (F0, F1, F7) flips too; a limit that stands.

## Verification

- Task verifies, run as written: Task 1 (`--stage video-check`, `--stage conformance`, RESULT greps video_bg_scroll and bg_scroll), Task 2 (same stages, RESULT greps video_bg_scroll_asym, video_bg_enab, video_fault_sweep_bg_enab, mutate_sweep_bg_scroll) and Task 3 (`--stage conformance`, `CONTROL
  vid-scroll-latch RED_OK`, RESULT greps chunk_equiv_video_bg_scroll and snapshot_restore_video_bg, `--stage video-check`): PASS. Full `sh scripts/check` PASS before each commit (about 4.3 min; the first run of Task 2 failed INV_PIN_BACKLINK and was fixed, see deviation 9). `sh scripts/check
  --selftest` PASS (145 PASS lines, no FAIL). `sh scripts/audit-history --range 607ad49..HEAD` clean (3 commits).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (70 s). Wave W5 end: `LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc' sh scripts/linux-repro` PASS (gate-gcc PASS, gate-clang PASS, core-boundary-gcc PASS; 139 s). Local one-off container runs, not a CI claim. `dash
  scripts/check` and the second-awk whole-gate runs are owed at the checkpoint plan (04-13) and the close-out.
- **Controls shown able to fail (ER-11):** the toy vid-scroll-latch (above); the three fault sweeps flip every declared fault of every group (bg_scroll: struct F4 F8 F9, latch F0 F1 F7 F8, latch_rel F0 F1 F7, byr_next F0 F1 F7 F8, byr_own F0 F1 F7, byr_rel F0 F1 F7; asym: wrap_x and wrap_y F0 F1 F7
  F8; enab: bb_on F0 F1 F7, burst and backdrop F0 F3, flags F0, toggle_own F0 F1 F7, toggle_next F0 F8); the mutation sweeps flip each of the 4 + 4 readback literals; the judge-forms rows show each new form failing on a wrong literal, on a write within 250 clocks of an origin and on a write that the
  log does not hold.
- **The hand-derived literals matched on the first run** that placed the writes where the literals assume (written before any DuoSpool output was read): the BXR rows (31, 28, 19, 0 shifted by two tiles), the BYR rows (row 64 in bank 2), the 64 x 64 wrap rows (index 0, 255, 12, 3 at line 0; 252, 243,
  0, 15, 12 and 243, 0, 15, 12, 3 at the lines 211 and 212) and the enable rows. The frame and line numbers (frame 8 of the wrap scene; frames 3, 5, 7, 9, 11 and 12 of the enable scene; the display line of each counted write) are calibrations read from the guest's own counted sequence, never literal
  sources: a wrong one fails its rows.
- **Evidence class E0 throughout** (repeatability). Classes (ER-10): the next-line rows of BXR and BYR MAN H7-7 (with H7-14 and H7-18 on solid tiles); the own-line rows, the counter rows and REL scroll_y_shift ASSUMED LED-VID-0009; scroll direction, the wrap and the 64 x 64 layout ASSUMED
  LED-VID-0010, never MAN (ER-12); tile rows with BB set and the own line of the toggle MAN H7-6; the burst frame, the backdrop and the lines after the toggle ASSUMED LED-VID-0010; flags and structure ASSUMED LED-VID-0004. No scroll-accurate, raster-accurate, pixel-accurate, compatibility or
  hardware claim; a pass is not a support claim; no ledger status changed.
- **Conformance stage time:** about 67 to 68 s locally (54 s before the toy and the restore case, 51 s before this plan); the trigger is 60 s. The additions are the restore case (about 6 s, 7,938 assertions) and the toy build and run (about 6 s); every video case is a single zero-policy run. See Notes for next plans.

## Deviations from Plan

1. **[Rule 3 - Blocking] The video judge needed the write join.** Not in the plan's file list: `tests/conformance/videokinds.c` (write selectors, margin guard, REL at-form, flat_frame and flat_at, judge-forms rows) and `scripts/lib/vexp-rules.awk` (the grammar). The dev scan recorded only HSR, HDR,
   CR and palette writes (the 04-11 note said so); BXR, BYR and the VDC CR data writes are now recorded in `t_vw` (16 register writes each).
2. **[Rule 3 - Blocking] `tests/conformance/obskinds.c` OX_ROMS_MAX 32 to 40.** The opcode_exercise inventory refused the 33rd guest ROM (bg_scroll) with a harness_failure; bg_enab is the 34th, so six more fit. Not in the plan's list.
3. **[Note] BXR = 16, not 8** (key decision 1). The plan's "shifted by 8 dots = one tile" cannot be both direction-independent (MAN) and non-vacuous.
4. **[Note] The bank rule of the 32 x 32 BAT changed between Task 1 and Task 2** (bank 1 for rows 0 to 7, then (r & 3) + 2) so that the REL scroll_y_shift rows cross a bank change (F0 on dy flips them), and the second BYR write was moved from 8 to 20 lines after the first for the same reason;
   bg_scroll.s therefore changed after the Task 1 commit and its manifest sha was repinned in Task 2. The BXR rows of Task 1 were not affected (banks of rows 0 to 7).
5. **[Note] A second fault sweep case** `video_fault_sweep_bg_scroll_asym` (one scene per sweep case; the plan lists one sweep file for bg_scroll). Also `snapshot_restore_video_bg` and the cases are the plan's; no other file was added.
6. **[Note] REL blank_is_border is not used.** A video case has one run (`main`), and the burst frame is a flat line set, which flat_frame judges; the relation stays tested by the judge-forms rows of 04-07.
7. **[Note] The burst frame is classed ASSUMED LED-VID-0010, not MAN H7-6** (key decision 4).
8. **[Note] F8 is not declared by groups whose first row sits on the first display line** (bb_on, wrap_x row 0): the swap moves the line one record earlier and the row still finds the same content; wrap_x starts at line 4 (its predecessor shows another BAT row) so F8 flips it, and bb_on declares F0, F1 and F7.
9. **[Note] `video_bg_scroll_asym` has `pins = []`**: the ledger check INV_PIN_BACKLINK requires a pinned case to be the pinning_test of the entry, and LED-VID-0010 keeps TEST:video_bg_basic_asym as the plan says; LED-VID-0010 names video_bg_scroll_asym and video_bg_enab as witnesses in a comment.
   LED-VID-0009 pins TEST:video_bg_scroll (it was PENDING:4). VIDEO_TOY_EXEMPT gets lines for video_bg_scroll_asym and video_bg_enab (video_bg_scroll is named by the toy EXPECT).
10. **[Note] Plan estimate was 95,000 tokens; the realized diff is about 24,300** by chars/4 over the added lines (30 files, 1,164 insertions, 16 deletions). The stop rule did not fire.
11. **[Note] The standard plan-complete step may tick VID-01 and VID-06** in REQUIREMENTS.md (see the close-out note).

## Known Stubs

None. No fixture registers or data in this plan are placeholders. Not yet exercised by any fixture: SCREEN sizes other than 000 and 101, sprites over the backdrop (04-14), the bg_off_index with a sprite covering a dot.

## Threat Flags

None. T-04-41 (scroll offsets and wrap arithmetic): the 10-bit BXR and 9-bit BYR masks and the power-of-two modulo of the virtual column and row were already in place (04-11); the wrap phase (BXR 500, BYR 300, a 64 x 64 BAT) now runs under the sanitizers in the full gate. T-04-42 (claim inflation):
the next-line rows are MAN H7-7 only, the first-display-line row, the direction and the wrap are ASSUMED, no sub-line offset is claimed, and the toy proves the BYR latch literal live. T-04-43: chunk_equiv_video_bg_scroll (1364, 1366) and snapshot_restore_video_bg compare stream bytes with memcmp
around the counted writes.

## Notes for next plans

- **The judge now has the write join** for any later latch row: `line=bxr|byr|cr N+D` in a ROWS selector (N counts the guest's register writes from 0, setup included), `;at=NAME N` and `;fa=prev` on a REL selector, and the STRUCT checks `flat_frame` (index, frame, from, count, burst) and `flat_at`
  (index, reg, wr, d, count). The dev scan holds 16 register writes of each of BXR, BYR and CR (`VK_VW_MAX` 32 data writes); a guest that writes more must raise it. A latch fixture places its write with a counted delay and the harness message prints the two offsets when a write is within 250 clocks
  of an origin (a calibration step, no literal comes from it).
- **bg.inc was used unchanged; DSV ids 23 and 24 are taken, the next free id is 25.** `OX_ROMS_MAX` is 40 with 34 guest ROMs in use. Editing bg_scroll.s or bg_enab.s moves the image (manifest sha, frame and line calibrations in the vexp, the restore window of snapshot_restore_video_bg, which ends just after the last counted write at about 1,552,000 clocks).
- **Conformance stage time is about 67 s locally** (trigger 60 s). The restore case is the one lever left that costs seconds (a run of 1.56 million clocks restored at every stop of two schedules under three policies); the toy builds are the other. Later plans should stage or trim before adding more, or the orchestrator may move snapshot_restore_video_bg to a staged job.
- **04-13 (checkpoint)** owes `dash scripts/check` and the second-awk whole-gate runs; the W5 Linux parts were run here and passed.
- `.planning/state.json` was left untouched and uncommitted.

## Close-out note on requirement ticks

The standard plan-complete step (`requirements.mark-complete`) ticks VID-01 and VID-06; 04-16 lists all six ids of the phase and has no SUMMARY yet, so any tick was reverted with a file-specific `git checkout` of .planning/REQUIREMENTS.md after checking that nothing else in that file was modified; REQUIREMENTS.md equals HEAD and the normal step of 04-16 ticks them.

## Self-Check: PASSED

- FOUND: fixtures/src/bg_scroll.s, bg_scroll.checks, bg_scroll.vexp, fixtures/manifest/bg_scroll.toml, the same four for bg_enab, the twelve new case files, tests/controls/vid-scroll-latch (RULE and EXPECT), REQUIRED lines for the 12 new ids.
- FOUND commits 864f7f3, 7498e5e and f92f162 (ancestors of HEAD).
