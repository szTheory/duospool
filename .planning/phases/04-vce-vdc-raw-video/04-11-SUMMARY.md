---
phase: 04-vce-vdc-raw-video
plan: 11
subsystem: vdc-background-renderer
tags: [vid, background, bat, cg, palette-lookup, bg-y, unmodelled, bit-order, symmetry-split, video-judge, frame-selector, bg-basic, w5]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the one VRAM accessor pair and the register port (04-06), the vertical machine and the MWR latch at VBlank entry (04-08), the active window and the blank payload (04-09), the burst latch and the DMA (04-10), the video judge, the sweep and the toy machinery (04-05, 04-07, 04-17)
provides:
  - src/core/vid.c - the background composition inside the const render: BAT size from the SCREEN bits of the latched MWR (two static const tables), CG0 and CG1 planes, index 0 for pixel 0 whatever the bank and (bank << 4) | px otherwise, the palette lookup at the line event, burst gives index 256,
  BB = 0 without burst gives bg_off_index, the flags BB (b2), SB (b3) and UNMODELLED (b4)
  - src/core/beam.c and vdc.c - the bg_y counter (BYR at display line 0, plus one modulo 512 at each later display line) and the BYR write that sets it, each one statement
  - src/core/constants.def - bg_off_index 0 (LED-VID-0010); both accept goldens and record_skeleton_256k changed by hand from the def (34 withheld ids, 151 skeleton rows)
  - tests/conformance/videokinds.c and scripts/lib/vexp-rules.awk - the optional ;frame=F item of a ROWS selector and the STRUCT check frame_flags, with judge-forms selftests
  - fixtures/lib/bg.inc (macros and equates only), fixtures/src/bg_basic.s (id 22, 6 readback checks), its checks, vexp and manifest; the cases bg_basic, video_bg_basic_solid, video_bg_basic_asym, video_unmodelled_mode, video_bg_basic_golden, three fault sweeps and mutate_sweep_bg_basic
  - tests/controls/vid-bit-order (the symmetry-split toy) and the ledger pins LED-VID-0010 (TEST:video_bg_basic_asym) and LED-VID-0015 (TEST:video_unmodelled_mode)
affects: [04-12, 04-13, 04-14, 04-15, 04-16]

requirements-advanced: [VID-01]
requirements-completed: []

actuals:
  tokens: 14900
  tasks: 3
  commits: 3
plan_head_before: 8ab04afa0d183e60a937a2d3089f9ee341716263
plan_head_after: 9bc7a293c880b7cd385ca8f32ee92fcd689390e2

tech-stack:
  added: []
  patterns:
    - "A literal row that must select a frame the guest sets up over several frames carries ;frame=F, and a wrong frame can only fail: no other frame holds the hand-derived indices, so the frame numbers are a calibration read from the guest's own counted sequence and never a literal source"
    - "The symmetry split by construction (ER-12): every tile of the MAN group is solid (pixel 15, 12, 3 or 0), so no bit order, byte order or flip changes it; the ASSUMED group holds the one-pixel diagonal and the pixel 1 and 2 tiles. One toy changes the bit order and the two cases separate in the controls file (EXPECT with a pass column)"
    - "A frame whose first record follows the latch is judged by a flag of every record (frame_flags): the check refuses a frame the stream does not hold whole as a fault of the case, never a pass"

key-files:
  created:
    - fixtures/lib/bg.inc
    - fixtures/src/bg_basic.s
    - fixtures/src/bg_basic.checks
    - fixtures/src/bg_basic.vexp
    - fixtures/manifest/bg_basic.toml
    - tests/conformance/cases/bg_basic.toml
    - tests/conformance/cases/video_bg_basic_solid.toml
    - tests/conformance/cases/video_bg_basic_asym.toml
    - tests/conformance/cases/video_unmodelled_mode.toml
    - tests/conformance/cases/video_bg_basic_golden.toml
    - tests/conformance/cases/video_fault_sweep_bg_basic.toml
    - tests/conformance/cases/video_fault_sweep_bg_basic_asym.toml
    - tests/conformance/cases/video_fault_sweep_unmodelled_mode.toml
    - tests/conformance/cases/mutate_sweep_bg_basic.toml
    - tests/controls/vid-bit-order/RULE
    - tests/controls/vid-bit-order/EXPECT
  modified:
    - src/core/vid.c
    - src/core/beam.c
    - src/core/vdc.c
    - src/core/constants.def
    - tests/conformance/videokinds.c
    - scripts/lib/vexp-rules.awk
    - tests/conformance/REQUIRED
    - tests/conformance/VIDEO_TOY_EXEMPT
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/controls/vid-palette-mask/EXPECT
    - ledger/entries/LED-VID-0010.toml
    - ledger/entries/LED-VID-0015.toml

key-decisions:
  - "bxr_line is read from the BXR register inside the render, at the line event, as one statement; no new instance field exists, so the snapshot layout and the state serialisation are unchanged. The plan's beam.c latch of BXR is therefore a local of vid.c; the record field bxr_eff carries the 10-bit value used"
  - "bg_y is stepped in beam.c at every display line after the vertical machine and the RCR counter (display_k 0 takes BYR, later lines add one modulo 512), with or without a sink; a BYR write in vdc.c sets it to the written 9-bit value, so a write during line n shows BYR + 1 on line n + 1 (the anchor 04-12 needs for the vid-scroll-latch toy, a single statement)"
  - "The flags BB and SB are CR bits 7 and 6 at the line event on every record (display or not); UNMODELLED is set on every record while the latched MWR has VM (bits 1-0) or SM (bits 3-2) non-zero; the pixels are drawn as VM = 0 and SM = 0 whatever the bits"
  - "The pixel bit extraction is the one statement uint32_t bit = 7u - (col & 7u); of bg_index, which is the toy anchor; the plane order (low byte plane 0 or 2, high byte plane 1 or 3) is the next statement and rests on the same ASSUMED LED-VID-0010"

completed: 2026-11-12
---

# Phase 4 Plan 11: Background renderer Summary

**The const render now draws the background: a dot takes the BAT word of its virtual column and row, the tile it names (16 words at code * 16, CG0 and CG1), the index 0 for pixel 0 whatever the bank and (bank << 4) | px otherwise, and the palette value of that index at the line event, with burst
giving 256, BB off giving bg_off_index and VM or SM set giving the UNMODELLED flag; a 256-wide fixture is judged by hand-derived rows split into an order-insensitive MAN group and a separate ASSUMED group, and the toy vid-bit-order turns only the second red.**

## What was built

- **Task 1, tracer (commit 7f9e8df).** `vid.c`: `bg_index`, the static const BAT size tables, the burst, BB-off and background branches of the payload loop, the flags BB, SB and UNMODELLED, `bxr_eff` from the 10-bit BXR; `beam.c`: `bg_step`; `vdc.c`: the BYR write; `constants.def` bg_off_index 0
  with both goldens and the skeleton by hand. The video judge gained `;frame=F` on a ROWS selector and the STRUCT check `frame_flags` (kind, grammar and judge-forms selftests). The fixture `bg_basic` (id 22: palette bijection by `VCE_WRITE_P91`, a 32 x 32 BAT, seven hand-packed tiles uploaded by
  TIA, six GUEST readback checks)
  and the scene `video_bg_basic_solid`: index rows MAN H7-18 (8 dots each of 63, 12, 243 and 0, then 15, over display lines 0 and 7), value rows GUEST pal_bijection (118, 85, 114, 17, 358), the border 273 and the structure rows. The scene is killed by the toy vid-palette-mask (its EXPECT also names bg_basic and the solid case, both go red on an 8-bit palette store).
- **Task 2 (commit f312f7c).** The asymmetric scene `video_bg_basic_asym` (the one-pixel diagonal at the dots a bit-7-leftmost reading gives on tile rows 0, 3 and 7, and the pixel 1 and 2 tiles in the order a low-byte-plane-0 reading gives), the scene `video_unmodelled_mode` (UNMODELLED on every
  record of frame 10, clear on every record of frame 7, pixels of frame 10 drawn as VM = 0), the golden row beside the literal scenes, the three fault sweeps and `mutate_sweep_bg_basic`. The unmodelled phase is the second phase of the same ROM, so no variant image was needed.
- **Task 3 (commit 9bc7a29).** `tests/controls/vid-bit-order` (one statement of vid.c, EXPECT `video_bg_basic_asym` red and `video_bg_basic_solid` TAB pass), the VIDEO_TOY_EXEMPT lines for the golden and the unmodelled scene, and the ledger pins.

## Verification

- Task verifies, run as written: Task 1 (`--stage video-check`, `--stage conformance`, RESULT greps video_bg_basic_solid and vid_noninterference), Task 2 (same stages, RESULT greps video_bg_basic_asym, video_fault_sweep_bg_basic, video_unmodelled_mode, mutate_sweep_bg_basic) and Task 3 (`--stage
  conformance`, `CONTROL vid-bit-order RED_OK`, `--stage video-check`, `--stage ledger-entries`, `--selftest mutation-control-anchor`): PASS. Full `sh scripts/check` PASS before each of the three commits (about 4.2 to 4.7 min). `sh scripts/check --selftest` PASS (144 PASS lines, no FAIL).
  `sh scripts/audit-history --range 8ab04af..HEAD` clean (3 commits).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, -Wconversion -Wshadow -Werror, linux/amd64, 55 s); a one-off local run, not a CI claim. `dash scripts/check` and the second-awk whole-gate runs are owed at the Phase 4 checkpoint and close-out.
- **Conformance stage time:** about 50.8 s locally after this plan (56.5 s before it; the stage has nine more cases and each of them runs one guest); the trigger is 60 s. The video fixtures use the zero policy only (one run per case) and the guest case bg_basic runs the three policies once, so no further lever was needed.
- **The hand-derived literals matched on the first run** (written before any DuoSpool output was read, derivations in the vexp): P(63) = 118, P(12) = 85, P(243) = 114, P(0) = 17, P(15) = 358, border 273, the diagonal at dots 8, 11 and 15, the pixel 1 and 2 tiles as 65 and 66. The frame numbers (5
  for the scenes, 7 and 10 for the unmodelled flag) were read from the guest's own counted sequence on one run as a calibration (frames 0 to 2 setup and burst, 3 to 8 the background, the MWR write in frame 8 before its VBlank entry, 9 to 11 wholly latched).
- **Controls shown able to fail (ER-11), each on the built runner with vid.c restored byte for byte (cmp):** the toy vid-bit-order turns video_bg_basic_asym red and leaves video_bg_basic_solid green (RED_OK); the index kept for pixel 0 (bank not ignored) turns the solid, asymmetric and unmodelled
  scenes red; the plane byte order swapped (pixel 1 and 2 exchanged) turns video_bg_basic_asym red and leaves video_bg_basic_solid green; a bank shift of 3 turns the solid and unmodelled scenes red; vid-palette-mask (an 8-bit palette store) turns bg_basic and video_bg_basic_solid red.
  The three sweeps flip every declared fault of every group (solid F0 F1 F7, solidval F0 F1, bitorder F0 F1 F2 F3 F4 F7 F8, planes F0 F1 F2 F4 F7 F8, pix F0 F1 F7, flag F0, struct F4 F8 F9, border F0); mutate_sweep_bg_basic flips each of the 6 readback literals.
- Evidence class E0 throughout. Classes (ER-10): the index rows of the solid group MAN H7-18 with the layout of H7-14 and H7-15 (every tile solid, so no pixel order is assumed); the value rows and the border GUEST pal_bijection (the guest's additive readback, checks 5 and 6); the asymmetric rows
  ASSUMED LED-VID-0010 (they also rest on the row counter of LED-VID-0009); the UNMODELLED rows ASSUMED LED-VID-0015 (deferred); the structure rows ASSUMED LED-VID-0004. No pixel-accuracy, raster-accuracy, compatibility or hardware claim;
  a pass is not a support claim; no ledger status changed.

## Deviations from Plan

1. **[Note] The final fixture image is in the Task 1 commit.** The plan spreads bg_basic.s over Tasks 1 and 2 ("first slice", then "extend"); the ROM was written whole (all seven tiles, the second BAT row, the unmodelled phase and six checks) so the image, its manifest pin and the case check counts
  are pinned once. Task 2 changed no fixture source, only the vexp scenes, the cases and the golden. Task 1 was green on its own.
2. **[Note] `bxr_line` is a local of vid.c, not a latch in beam.c.** The render runs at the line event, the same instant, so no instance field and no snapshot change were needed (see key decisions).
3. **[Rule 3 - Blocking] The video judge was extended.** `vk_active_lines` took the first frame that has an active window, which is frame 0 (the power-on cycle draws 8-dot display lines before the guest programs anything), so no ROWS row could reach the scene the guest sets up later. Added the
  optional `;frame=F` selector item (`videokinds.c`, `vexp-rules.awk`, three judge-forms rows) and the STRUCT check `frame_flags` (want, frame, bit) for the UNMODELLED flag, with judge-forms rows. Files not in the plan's list: `tests/conformance/videokinds.c`, `scripts/lib/vexp-rules.awk`.
4. **[Rule 3 - Blocking] The guest counts frames instead of polling the VBlank flag.** A polling loop filled the 16,384-record device log (every status read is logged, and the sink check then refuses the run); the fixture uses counted delays (`BG_FRAME`, 119,210 CPU cycles at the high speed, the
  same loop as vce_dot) and the frame numbers are measured from the run, a calibration that can only fail a row, never pass it.
5. **[Note] `tests/controls/vid-palette-mask/EXPECT` is edited** (not in the plan's list): the scene video_bg_basic_solid had to be named by some toy EXPECT without a pass column, and bg_basic and the solid case are the natural victims of an 8-bit palette store (the palette readback and the value
  rows). The vid-bit-order EXPECT keeps the solid case in its pass column as the plan says.
6. **[Note] Three fault sweeps, not one.** The sweep kind judges one scene per case, and the plan has three literal scenes (solid, asymmetric, unmodelled): `video_fault_sweep_bg_basic`, `video_fault_sweep_bg_basic_asym` and `video_fault_sweep_unmodelled_mode`. The flag group of the unmodelled scene
  declares F0 only (F3 would make frame 10 a partial frame, a fault of the case, and F4 leaves it whole), as the plan's Task 3 text says.
7. **[Note] Six GUEST readback checks** (the plan names guest-side readbacks of the BAT and tiles): the BAT fields by shifts (2), the tiles against the ROM table (2) and the palette against the additive sum (2). `GUEST:pal-bijection` is the spelling of the .checks class (the lint wants a hyphen there); the vexp class is `GUEST:pal_bijection` (the allowlisted model).
8. **[Note] The vce_pal golden did not change.** The vce_pal guest leaves every vertical register at 0 (a 4-line cycle with one display line), so bg_y stays 0 there, its frames are burst frames (blank payload) and no flag bit it sets is new: `video_vce_pal_golden` still passes on the old digest, with no repin.
9. **[Note] Plan estimate was 110,000 tokens; the realized diff is about 14,900** by chars/4 over the added lines (30 files, 719 insertions, 34 deletions). The stop rule did not fire.
10. **[Note] The standard plan-complete step may tick VID-01** in REQUIREMENTS.md (see the close-out note).

## Known Stubs

None that this plan adds. Not yet exercised by any fixture: bg_off_index (SB set, BB clear, no burst), the SCREEN sizes other than 000, a non-zero BXR or BYR, the wrap of a virtual column or row; they are defined by the renderer and the tables, and the enable matrix and the scroll scenes belong to
04-12. Sprites do not exist before 04-14, so with BB = 0 and no burst every active dot takes bg_off_index.

## Threat Flags

None. T-04-38 (guest-chosen codes, banks, SCREEN sizes and scroll offsets) is mitigated: every VRAM word goes through the bounds-checked accessor (a tile code above 2047 addresses past the 32,768-word VRAM and reads 0), the SCREEN index is three bits into two 8-entry static const tables, the column
and row wrap by masks of the power-of-two sizes, and bg_y is kept to 9 bits at both writers; the sanitizers stage ran bg_basic. T-04-39: the render still takes a const instance and the pixel part is gated on the sink (vid_noninterference and the toy vid-emit-mutates stay live and green). T-04-40:
the asymmetric group is ASSUMED only (video-check VEXP_ASYM_CLASS) and the toy vid-bit-order proves the solid group order-insensitive.

## Notes for next plans

- **04-12 (scroll and the enable matrix)** can use `fixtures/lib/bg.inc` unchanged (BG_REGS_256, BG_FILL_BAT, BG_HOLD, BG_FRAME, BG_CMP_WORD and the register equates) and the DSV id 22 is taken, the next free id is 23. A fixture that needs a status poll must keep its device-log records below 16,384
  (polls are logged), or use counted delays like bg_basic. Judge frames with `;frame=F` and calibrate F from one run; put margin frames on both sides.
- **The dev scan records only HSR and HDR data writes** (and CR, palette): BXR and BYR writes need to be added to the scan before a row joins them to a line by stamp (04-12). The BYR write path is the single statement `inst->bg_y = ...` in vdc.c (anchor of vid-scroll-latch); bg_y is stepped in `bg_step` of beam.c.
- **Editing bg_basic.s, bg.inc or the tile table moves the image** (manifest sha, the three frame numbers in the vexp, the check counts of the cases, the golden). The golden digest of the stream is 9903974543189138e84ab27f2df44412d716553c2d46a28f62f1daf2642c735a; any plan that changes a record field
  of a burst-free frame repins `video_bg_basic_golden` (independent `shasum -a 256` over the --video-out file; `mkdir -p build/gate/video` first, remove it after).
- **Editing `bg_index` or the vexp rows** requires re-deriving the rows; the vid_active_window and vce_dot rows are untouched (their frames are burst frames, index 256).
- **Owed:** `dash scripts/check` and the second-awk whole-gate runs at the Phase 4 checkpoint and close-out. The conformance stage is about 50.8 s locally (trigger 60 s); later plans should keep new cases lean and use the D-176 levers (fewer policies for video fixtures) before it grows.
- `.planning/state.json` was left untouched and uncommitted.

## Close-out note on requirement ticks

The standard plan-complete step (`requirements.mark-complete VID-01`) ticked the VID-01 checkbox and its traceability row. This plan only advances VID-01 (the scroll latch of 04-12 completes it) and 04-12 and 04-16 still list it, so the tick was reverted with a file-specific `git checkout` of
.planning/REQUIREMENTS.md (nothing else in that file was modified); REQUIREMENTS.md equals HEAD and the normal step of 04-16 ticks it.

## Self-Check: PASSED

- FOUND: fixtures/lib/bg.inc, fixtures/src/bg_basic.s, bg_basic.checks, bg_basic.vexp, fixtures/manifest/bg_basic.toml, the nine new case files, tests/controls/vid-bit-order (RULE and EXPECT), REQUIRED lines for the 9 new ids.
- FOUND commits 7f9e8df, f312f7c and 9bc7a29 (ancestors of HEAD).
