---
phase: 04-vce-vdc-raw-video
plan: 09
subsystem: horizontal-geometry-and-dot-clock
tags: [vce, vdc, active-window, clip, dot-clock, line-records, vce-dot, vid-frame-lines, vid-active-window, vid-dot-div, fault-sweep, w4]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the vertical machine and the line records (04-04, 04-08), the video judge, the .vexp grammar and the fault sweep (04-07), the dev log with write stamps (04-19), the toy machinery (04-17)
provides:
  - src/core/vid.c - the active window of a display line from HSR, HDR and the divider latched at EV_LINE (active_x = (HDS+1)*8, active_w = (HDW+1)*8 dots, clipped to the whole dots that fit with the CLIPPED flag, active_x 0 when no dot fits), the blank payload (index 256 and the colour of palette entry $100) and rec_bytes from the window
  - fixtures/src/vce_dot.s (id 19, 4 checks) - the vertical setup, HSR/HDR, the border entry by an additive sum, eight CR writes (00, 01, 02, 03, a clipped setup, a window beyond the line, the base setup, a counted mid-frame CR and HDR change), HSR and HDR writes in the same style; one ROM, three scenes
  - the scenes video_vce_dot, vid_frame_lines (request bind.lines_per_frame=263) and vid_active_window in fixtures/src/vce_dot.vexp, with three fault sweeps, the guest case vce_dot and mutate_sweep_vce_dot
  - the STRUCT checks div_window, window, display_flags, line_dirty_hv and cr_change in tests/conformance/videokinds.c (and in scripts/lib/vexp-rules.awk), the dev scan of HSR and HDR data writes by AR
  - the toy vid-dot-div and the ledger pins LED-VID-0001 (vid_frame_lines), LED-VID-0002 and LED-VID-0017 (video_vce_dot), LED-VID-0003 and LED-VID-0005 (vid_active_window); statuses stay assumed
  - the vce_pal golden repinned
affects: [04-10, 04-11, 04-12, 04-13, 04-15, 04-16]

requirements-advanced: [VID-06]
requirements-completed: []  # VID-05 and VID-06 are listed by 04-16 as well, which completes all six; the tick waits for it (A9, late tick rule)

actuals:
  tokens: 16700
  tasks: 3
  commits: 3
plan_head_before: f5e73660b762c915c87ef54a979476e5c119bf23
plan_head_after: 3848966c0a37fe9a14cac20b7118fac7216deb89

tech-stack:
  added: []
  patterns:
    - "A window row is judged on the display lines inside the window of one CR write (at least one line after it, two lines before the next), so a setup is a CR write plus the registers written before it, held for about a frame; every setup is closed by a CR write even when its value does not change"
    - "An instant that has to be far from a line boundary is not trusted to a hand count: the runner finds the line of the write by the record origins and refuses the case (harness_failure) when the stamp is within the margin of a boundary, so a later timing change cannot make the row silently depend on the tie rule"
    - "The VDC data writes are joined by the AR the guest last set (dev log, DS_DEV_ST for ST0 to ST2): a register write has two stamps, the low byte and the high byte, and each sets the dirty bit of its own line, so a boundary between them gives two flagged records"

key-files:
  created:
    - fixtures/src/vce_dot.s
    - fixtures/src/vce_dot.checks
    - fixtures/src/vce_dot.vexp
    - fixtures/manifest/vce_dot.toml
    - tests/conformance/cases/vce_dot.toml
    - tests/conformance/cases/mutate_sweep_vce_dot.toml
    - tests/conformance/cases/video_vce_dot.toml
    - tests/conformance/cases/vid_frame_lines.toml
    - tests/conformance/cases/vid_active_window.toml
    - tests/conformance/cases/video_fault_sweep_vce_dot.toml
    - tests/conformance/cases/video_fault_sweep_vid_frame_lines.toml
    - tests/conformance/cases/video_fault_sweep_vid_active_window.toml
    - tests/controls/vid-dot-div/RULE
    - tests/controls/vid-dot-div/EXPECT
  modified:
    - src/core/vid.c
    - src/core/beam.c
    - src/core/vce.c
    - tests/conformance/videokinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/cases/vid_chan_append.toml
    - tests/conformance/REQUIRED
    - tests/conformance/VIDEO_TOY_EXEMPT
    - scripts/lib/vexp-rules.awk
    - fixtures/src/vce_pal.vexp
    - ledger/entries/LED-VID-0001.toml
    - ledger/entries/LED-VID-0002.toml
    - ledger/entries/LED-VID-0003.toml
    - ledger/entries/LED-VID-0005.toml
    - ledger/entries/LED-VID-0017.toml

key-decisions:
  - "The window is computed in vid.c from the registers as they stand at EV_LINE (the render runs inside EV_LINE, so reading them there is the latch), not in beam.c; beam.c only gained a comment. The clip keeps the dots whose end is within 1365 clocks: floor(1365 / div) - active_x, none when that is not positive, in which case active_x is 0 and CLIPPED is still set"
  - "Display lines are the records with the ACTIVE flag (the vertical state VDW of 04-08); a display line with no dot (window beyond the line) is still a display line with rec_bytes 64, and every other line is header-only"
  - "The vce_pal golden moved with Task 1 (not Task 3 as the plan lists it): Task 1 already changes every display line of the vce_pal stream, so the golden row needed the new digest for that commit to be green; one repin, old and new digest in the vexp comment, recomputed with shasum -a 256 over the --video-out file after the literal case video_vce_pal passed"
  - "vk_s_div_window takes the dividers by the CR write they belong to (div=...;at=...;crw=...), so the first number of a literal is a judged divider and the fault F0 flips a group that judges only the contents 02h and 03h"
  - "width_set is judged with min_distinct=5 (0, 8 before HDR is programmed, 256, 333, 320); it is a lower bound, so its only kill is F0 and its liveness is the comparison, as the vce_pal border group (A7)"
  - "vid_frame_lines judges the 263 binding; the 262 default is judged by the frame, display and struct groups of video_vce_dot"

completed: 2026-11-12
---

# Phase 4 Plan 09: Horizontal geometry and dot clock Summary

**A display line now carries the active window the registers program ((HDS+1)*8 dots from (HDW+1)*8 dots, clipped to the whole dots that fit with the CLIPPED flag) and a blank payload of index 256, a guest that runs eight setups (the four CR encodings, a clipped window, a window beyond the line, and
a counted mid-frame CR and HDR change) is judged by hand-derived rows in three scenes, every group is shown live by a record-fault sweep, and the toy vid-dot-div turns the divider rows red.**

## What was built

- **Task 1, tracer (commit 0e0f532).** `vid.c`: `active_window` (u32 arithmetic, at most (1024 + 1024) * 4), the payload loop (u16 index[active_w] = 256, u16 colour[active_w] = palette entry $100, zero padding to a multiple of 8) and rec_bytes; the record is built in the existing stack buffer of
  2,792 bytes and the window cannot exceed DS_VID_MAX_DOTS (clamped, a visible fact after the clip). `vce_dot.s` first slice (HSR $0202, HDR $031F, vertical setup of 264 lines, border entry by the additive sum E(256) = 273, CR $00, two frames), the scene video_vce_dot with groups struct, frame, div
  (MAN H6-3), geom (ASSUMED LED-VID-0005) and border (GUEST pal_bijection), the checks `div_window` and `window` of the judge. The vce_pal golden was repinned and `vid_chan_append` adapted (deviations 1 and 2).
- **Task 2 (commit 6de1524).** `vce_dot.s` complete: eight CR writes, nine HSR and HDR register writes, a counted delay before the mid-frame CR $01 and HDR $0327. `videokinds.c`: the dev scan now also follows the VDC window (AR, then the data writes with AR = HSR or HDR; `DS_DEV_ST` for ST0 to ST2),
  `VK_CRW_MAX` 12, `VK_KV_MAX` 12, and the checks `display_flags`, `line_dirty_hv`, `cr_change`. The scenes vid_frame_lines (263 lines) and vid_active_window with the hand table, the three fault sweeps and `mutate_sweep_vce_dot`.
- **Task 3 (commit 3848966).** `vce.c` keeps the first two divider entries on one line (the one anchor), the toy vid-dot-div (EXPECT video_vce_dot and vid_active_window), VIDEO_TOY_EXEMPT keeps only vid_frame_lines, five ledger pins.
- **Until 04-10 the blank payload of index 256 appears without the BURST flag.** In a frame with BB = SB = 0 D-157 says every active dot is index 256 and the frame is burst; the burst latch arrives in 04-10, so for now the record carries the blank payload with the BURST flag clear. This is an
  intended and temporary inconsistency (plan-check info 6); 04-10 sets the flag and 04-11 replaces the payload by the background.

## Verification

- Task verifies, run as written: Task 1 (`--stage video-check`, `--stage conformance`, RESULT greps video_vce_dot and video_vce_pal), Task 2 (the RESULT greps vid_active_window, vid_frame_lines, video_fault_sweep_vce_dot, mutate_sweep_vce_dot) and Task 3 (`--stage conformance`, `CONTROL vid-dot-div
  RED_OK`, `--stage video-check`, `--stage ledger-entries`, `--stage inventory`): PASS. Full `sh scripts/check` PASS before each commit (57 stages, about 4.2 min). `sh scripts/check --selftest` PASS (no FAIL). `sh scripts/audit-history --range f5e7366..HEAD` clean (3 commits).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, -Wconversion -Wshadow -Werror, linux/amd64, 54 s); a one-off local run, not a CI claim. `dash scripts/check` and the second-awk whole-gate runs are owed at the Phase 4 checkpoint and close-out; the wave-end Linux parts run after 04-10, not here.
- **Literals and controls shown able to fail (ER-11), each on the built runner with vid.c restored byte for byte (cmp):** the fault sweep of video_vce_dot flips all 26 declared faults of its 11 groups (struct F4 F8 F9, frame F0 F4, display F0 F3, div F0 F5, div2 F0 F5, geom F0 F5 F6, geom2 F0 F5 F6,
  midframe F0 F3 F5 F6 F8, widths F0, dirty F0 F4, border F0), the other two sweeps likewise; a window computed without the clip turns vid_active_window guest_failure (clip and beyond groups); an active_x written as the programmed value without the +1 (hds*8) turns video_vce_dot and
  vid_active_window red (geom, geom2, win31, clip, midframe); a CLIPPED flag that is never set turns vid_active_window red (clip, beyond) and leaves video_vce_dot green by design; the toy vid-dot-div turns video_vce_dot red (div, geom, midframe) and vid_active_window red (win31, clip, beyond) while
  the div2 and geom2 groups (div 2 entries) and the dirty rows stay green; mutate_sweep_vce_dot flips each of the 4 literals; the counted instant of the mid-frame change was shown to be guarded (a margin of 600 refuses the case with the measured offsets).
- **Measured values (calibration and evidence text, never a literal source):** the mid-frame CR write falls 444 master clocks after the origin of its line and the HDR write completes 624 clocks after it (line 1,365 long), so both are at least 250 clocks from both boundaries (margins 444 and 741);
  the delay count MIDDLY = 1 was chosen from this one measurement, and the margin guard in the runner (not the count) keeps it honest. The run ends at 3,459,780 master clocks (about 9.7 frames, until_mclk 4,000,000).
- Evidence class E0 throughout. div rows for the contents 00h and 01h are MAN H6-3 (judged only on records at least one whole line from the write), 02h and 03h ASSUMED LED-VID-0002, active_x/active_w/rec_bytes/CLIPPED and the display count ASSUMED LED-VID-0005, record counts, origins, frame length
  and line attribution ASSUMED LED-VID-0001 and LED-VID-0004, the border GUEST pal_bijection. No width is asserted as a hardware fact (320, 341 and 342 appear nowhere), no accuracy, compatibility, raster-accuracy or hardware claim; a pass is not a support claim; no ledger status changed.

## Deviations from Plan

1. **[Rule 3 - Blocking] The vce_pal golden repin is in Task 1, not Task 3.** Task 1 gives every display line a payload, which changes the vce_pal stream, so `video_vce_pal_golden` (REQUIRED) was red until the digest moved; Task 1 had to be green on its own. Old c667ac7a..., new 03fa9142... (vexp
   comment); the literal case video_vce_pal passed on the changed records first; recomputed with `shasum -a 256` over the --video-out file.
2. **[Rule 1 - Bug] `vid_chan_append` (not in the plan's list) expected 64-byte records.** Its attached part wrote bytes = 64 * count from the old header-only records. ds-proto programs no vertical or horizontal register, so the cycle is 4 lines and line k is a display line when k % 4 == 2, with an
   8-dot window (rec_bytes 96); the kind (obskinds.c) and the case text now compute bytes and the record shapes from that rule by hand (ASSUMED LED-VID-0005). No check was weakened: the header, guard, poison and non-interference rows are unchanged.
3. **[Note] src/core/vce.c is edited** (not in the plan's list): the first two divider entries share a line, because `scripts/control-mutate` anchors on one line and the toy must swap exactly those two. The table values and order are unchanged.
4. **[Note] beam.c has a comment change only.** The window is computed in vid.c from the registers at the time the render runs inside EV_LINE (the same instant as the latch), so the plan's beam.c edit was not needed.
5. **[Note] Files beyond the plan list:** `tests/conformance/obskinds.c` and `tests/conformance/cases/vid_chan_append.toml` (deviation 2), `scripts/lib/vexp-rules.awk` (the registry of the five new STRUCT checks and the grid-dependent set), the two sweep cases `video_fault_sweep_vid_frame_lines` and
   `video_fault_sweep_vid_active_window` (the sweep kind judges one scene per case, and the plan has three scenes), and the guest case `vce_dot` plus REQUIRED lines (8 ids). The ledger entry LED-VID-0001 also had its assumption sentence about the pin updated.
6. **[Note] vid_frame_lines judges the 263 binding only;** the 262 side of "records per frame equal 262 and 263" is the frame group of video_vce_dot (a video case runs one request).
7. **[Note] The mid-frame change also writes HDR** ($0327, 320 dots) beside CR $01, so that line n + 1 differs in div and in active_w (the plan's "div, active_x and active_w" cannot differ in dots through CR alone) and the width set has more than one non-zero element; both writes lie in one line (CR
   444 and HDR 624 clocks after the line origin). The CR-only half (div 4 to 3 at an unchanged window) is still in the rows (active_x 24 before and after).
8. **[Note] Plan estimate was 100,000 tokens; the realized diff is about 16,700** by chars/4 over the added lines (29 files, 821 insertions, 61 deletions). The stop rule did not fire.
9. **[Note] The standard plan-complete step may tick VID-05 or VID-06** in REQUIREMENTS.md (the plan lists them; 04-16 lists all six and completes them). Any early tick is reverted when it appears (see the close-out note at the end of this file).

## Known Stubs

The blank payload (index 256, every dot the border colour) is the intended interim result of D-157 for BB = SB = 0 frames; 04-11 replaces it by the background composition and 04-10 sets the BURST flag. Not a defect of this plan, recorded in the broken-windows ledger terms as a plan-scoped stub only.

## Threat Flags

None. T-04-31 (payload length from guest geometry) is mitigated: active_w is clipped to whole dots within 1365 clocks, at most floor(1365 / 2) - 8 = 674, clamped to DS_VID_MAX_DOTS 682 (static assert already in ds_obs.h), rec_bytes at most 2,792 and the stack record is the existing buffer of that
size; the sink append still refuses a record that does not fit. T-04-32 (stack buffer): the render writes at most rec_bytes <= DS_VID_MAX_REC_BYTES; the sanitizers stage ran vce_dot including the clipped window and the window beyond the line. T-04-33 (claim inflation): no width is asserted as
hardware, classes are per group, LED-VID-0017 keeps exactly the content 00h and 01h sentence.

## Notes for next plans

- **Records now carry payload.** A display line has rec_bytes 64 + ((4 * active_w + 7) & ~7); any plan that changes records repins `video_vce_pal_golden` in the same commit (independent `shasum -a 256` of the --video-out file; make `build/gate/video` first, remove it after). Current digest 03fa9142760f089c51cd33fc1a0e1cfec16547dbafcd652d40dc4cc23ddf8a85.
- **04-10** sets BURST and the blank rule for the enables; it must keep the blank payload identical for BB = SB = 0 (or repin). The vce_dot rows judge index 256 and colour = border on every display line, so a change of the payload for those frames turns video_vce_dot, vid_active_window and the sweeps red until the rows are re-derived.
- **Editing vce_dot later** moves the image (manifest sha, the checks file, the cases' `checks`, the sweep of its 4 literals). CR write numbering c0 to c7 and HSR/HDR write numbering h0 to h8 are in the header of vce_dot.s and the vexp; a new setup is appended, never inserted. The mid-frame delay
  MIDDLY is a calibrated count: if a later change of the fixture moves it, the margin guard of `cr_change` says so with the measured offsets.
- **New judge checks for later scenes:** `window` (display lines inside one CR write window: div, active_x, active_w, rec_bytes, CLIPPED, blank payload), `display_flags` (ACTIVE-flag count per complete frame from `skip`), `line_dirty_hv`, `cr_change`, `div_window`. The key order of a literal decides what F0 perturbs: put a value the group really judges first.
- **The dev scan** reads AR from writes to the VDC window ($1FE000 + offset & 3) and records data writes of HSR and HDR only; extend it for other registers when a later scene needs their stamps (BXR, BYR in 04-11).
- **Owed:** `dash scripts/check` and the second-awk whole-gate runs at the Phase 4 checkpoint and close-out; the wave-end Linux parts after 04-10. The conformance stage is now about 31 s locally (trigger 60 s).
- `.planning/state.json` was left untouched and uncommitted.

## Close-out note on requirement ticks

The standard plan-complete step (`requirements.mark-complete VID-05 VID-06`) ticked both checkboxes and both traceability rows, as it did for VID-04 in 04-08 and 04-17. 04-16 lists all six ids and completes them, so both ticks were reverted with a file-specific `git checkout` of .planning/REQUIREMENTS.md (nothing else in that file was modified); REQUIREMENTS.md equals HEAD and 04-16 ticks them through the normal step.

## Self-Check: PASSED

- FOUND: fixtures/src/vce_dot.s, vce_dot.checks, vce_dot.vexp, fixtures/manifest/vce_dot.toml, the eight new case files, tests/controls/vid-dot-div (RULE and EXPECT), REQUIRED lines for the 8 new ids.
- FOUND commits 0e0f532, 6de1524 and 3848966 (ancestors of HEAD).
