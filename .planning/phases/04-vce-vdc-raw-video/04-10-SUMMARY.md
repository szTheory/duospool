---
phase: 04-vce-vdc-raw-video
plan: 10
subsystem: vdc-dma-vram-to-vram-and-vram-to-satb
tags: [vdc, dma, satb, burst, ds, dv, lenr, dvssr, closed-form, vdc-advance-pure, snapshot-mid-dma, vdc-dma, vdc-satb, vdc-satb-data, vdc-stat, w4]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the register port, the one VRAM accessor pair and ds_vdc_flag (04-06), the vertical machine, the status flags and the IRQ1 level (04-08), guest interrupt measurements and toy machinery (04-17), the active window and the vce_pal golden repin rule (04-09), the DMA state layout and the restore window rules (04-03, 04-18, 04-19)
provides:
  - src/core/vdc.c - the LENR MSB write starts a VRAM-to-VRAM transfer (dma_begin), the DVSSR MSB write marks the SATB copy (satb_pending), ds_vdc_dma_apply (the closed form of the transfer, DV on completion), ds_vdc_satb_copy (256 words through the accessor pair) and ds_vdc_satb_due (DS at its time); the zero-divisor guards (a static assert on the dots per word, a runtime
    guard on dma_word_mclk and on lat_div)
  - src/core/beam.c - the words due and the DS time applied in time order before every event and at the target stamp, dma_line at EV_LINE (the burst latch at display line 0 as !(BB || SB), the cancel of a pending or running transfer in a frame that is not burst, the start of a pending transfer at the first line that finds the bus free), the SATB copy at the VBlank state entry
    (when satb_pending or DCR bit 4)
  - src/core/vid.c - the BURST flag on the display lines of a burst frame; src/core/constants.def vdc_dma_dots_per_word 4 (LED-VID-0014); both accept goldens and record_skeleton_256k changed by hand from the def
  - fixtures/lib/vdma.inc (macros and equates only), fixtures/src/vdc_dma.s (id 20, 24 checks), fixtures/src/vdc_satb.s (id 21, 9 checks), fixtures/src/vdc_stat.s extended to 21 checks (DV and DS rows 16 to 21)
  - the kinds vdc_advance_pure, vdc_satb_data and vdc_snapshot_mid_dma (vdckinds.c) and the cases vdc_dma, mutate_sweep_vdc_dma, vdc_dma_vram, vdc_advance_pure, vdc_satb, mutate_sweep_vdc_satb, vdc_satb_data, vdc_snapshot_mid_dma; the vce_pal golden repinned; ledger pins LED-VID-0013 (vdc_satb_data) and LED-VID-0014 (vdc_dma), statuses stay assumed
affects: [04-11, 04-12, 04-13, 04-14, 04-15, 04-16]

requirements-advanced: [VID-04]
requirements-completed: [VID-03]  # listed as completed by this plan; 04-16 also lists it, so the tick waits for 04-16 unless requirements.ready-ids says ready (see the close-out note)

actuals:
  tokens: 26300
  tasks: 3
  commits: 3
plan_head_before: 71535cfed1f79a1a87395a2fb5e9e1d4a688c973
plan_head_after: f89f6d41e18b0d2a6ab3aa322fb132803cdfcb83

tech-stack:
  added: []
  patterns:
    - "A DMA completion or a DS time is not a stored event: the fold applies the words due and the DS time by a closed form before every ladder event and at the target stamp, in time order, and the gate of ds_vdc_flag is read at that fold, which is the gate value of the completion instant because no access lies between the completion and the fold that applies it;
      ds_vdc_pending_time and the restore time windows stay as they were"
    - "A reference model that steps one master clock at a time with counters (no division, no shared formula) and takes its vertical cycle as plain arithmetic, so the closed form is compared with something that shares nothing with it; the lookahead samples one clock before, at and one clock after word boundaries and display starts"
    - "A restore stop chosen by machine state (dma_state, dma_done and the DCR step bits, satb_done_valid, satb_pending), the same shape as the vdc-tia stops of 04-06"
    - "An expected-red control by position: a check that reads the last destination word right after the start passes only while the copy takes time, so an instant transfer turns exactly that row red"

key-files:
  created:
    - fixtures/lib/vdma.inc
    - fixtures/src/vdc_dma.s
    - fixtures/src/vdc_dma.checks
    - fixtures/manifest/vdc_dma.toml
    - fixtures/src/vdc_satb.s
    - fixtures/src/vdc_satb.checks
    - fixtures/manifest/vdc_satb.toml
    - tests/conformance/cases/vdc_dma.toml
    - tests/conformance/cases/vdc_dma_vram.toml
    - tests/conformance/cases/mutate_sweep_vdc_dma.toml
    - tests/conformance/cases/vdc_advance_pure.toml
    - tests/conformance/cases/vdc_satb.toml
    - tests/conformance/cases/mutate_sweep_vdc_satb.toml
    - tests/conformance/cases/vdc_satb_data.toml
    - tests/conformance/cases/vdc_snapshot_mid_dma.toml
  modified:
    - src/core/vdc.c
    - src/core/beam.c
    - src/core/vid.c
    - src/core/ds_vdc.h
    - src/core/constants.def
    - tests/conformance/vdckinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/obskinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/model_param_vdc.toml
    - tests/conformance/cases/vdc_stat.toml
    - tests/conformance/cases/mutate_sweep_vdc_stat.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - fixtures/src/vdc_stat.s
    - fixtures/src/vdc_stat.checks
    - fixtures/manifest/vdc_stat.toml
    - fixtures/src/vce_pal.vexp
    - ledger/entries/LED-VID-0013.toml
    - ledger/entries/LED-VID-0014.toml

key-decisions:
  - "Progress is words due = (t - dma_start) / dma_word_mclk less the words done, at most the words that remain; the remaining words are LENR + 1 from the live register while the transfer runs, so no total is stored and a guest write to LENR mid-transfer is simply a live counter (D-151). Completion sets dma_state 0 and dma_done 0 and raises DV through ds_vdc_flag; a cancel
    sets the same two and raises nothing"
  - "A pending transfer holds the smallest legal word time (dots per word times 2) when the divider is not latched yet, so the restore rule (8, 12 or 16 while dma_state is not 0) holds for every reachable state; the real word time is latched from lat_div when it starts, at the EV_LINE that finds the bus free, and the first word is due one word time after that instant"
  - "The BURST flag is set on the display lines of a burst frame only (not on the VBlank or VDS lines before display line 0, where the latch still holds the previous frame): a flag is a fact about the display period it describes. The blank payload (index 256) is unchanged for every frame until 04-11"
  - "The SATB copy reads the live DVSSR at the EV_LINE of the VBlank state entry (after the burst latch and the pending start, before the render), stores each word little endian, clears satb_pending, leaves DVSSR and sets satb_done_at = that EV_LINE time + 256 * 4 * lat_div; no arbitration with the VRAM transfer (VL1)"
  - "The overrun case needs two VBlanks to pass after CR gets SB, because the first VBlank after a register write may belong to the power-on 4-line cycle; the transfer starts after the second, so the display line 0 of a whole frame has latched the burst state from CR and the cut at the next display start is judged"

completed: 2026-11-12
---

# Phase 4 Plan 10: VRAM-to-VRAM and VRAM-to-SATB DMA Summary

**A LENR high byte write now moves LENR + 1 VRAM words by a closed form that is applied before every event and every access, with the burst latch and the cancel at display start, DV and DS gated by DCR, and the DVSSR high byte arms a 256-word SATB copy at the next VBlank; the data is judged from guest readback and from instance bytes, the closed form against a per-clock
  reference over 256 random scenarios, and restores in the middle of both transfers continue identically.**

## What was built

- **Task 1, tracer (commit bbba639).** `vdc.c`: `dma_begin` (LENR MSB), `ds_vdc_dma_apply` (the closed form), `ds_vdc_satb_copy`, `ds_vdc_satb_due`, the DVSSR MSB arm; `beam.c`: `dma_line` (burst latch, cancel, pending start), the SATB copy at the VBlank state entry, the apply calls in the fold loop and at the target stamp; `vid.c`: BURST on display lines; `constants.def`
  vdc_dma_dots_per_word 4 with both goldens and the skeleton by hand (one constants row, LED-VID-0014 joins [withheld], 33 ids, 149 skeleton rows); `vdma.inc`; `vdc_dma.s` first slice (4 checks) with the cases vdc_dma and vdc_dma_vram; the vce_pal golden repinned. The core part of Tasks 2 and 3 (burst, cancel, SATB, DS) is in this commit because the golden changes with the
  first burst flag and the fold has to be complete to be green.
- **Task 2 (commit c8f5c2c).** `vdc_dma.s` complete: 24 checks in sections A to K (64 words against an additive running sum, the last word still old right after the start, LENR 0, SI/D, DI/D, DV gating and clear on read, IW ignored, an overlap, MAWR/MARR/buffer untouched, the LENR low byte alone and a lone high byte retrigger, an observed poll count, one overrun with SB set).
  `vdckinds.c` kind `vdc_advance_pure` (256 scenarios, seed 7, 14,440 comparisons, 92 cancels, 372 completions, 235 pending starts, plus the zero-divisor guard); `mutate_sweep_vdc_dma` (23 judged literals, check 21 excluded); `vdc_dma_vram` with 25 explicit words and the 64-word pattern.
- **Task 3 (commit f89f6d4).** `vdc_satb.s` (9 checks), the kinds `vdc_satb_data` (256 SATB words against `($A5A5 + $F1 * i) & $FFFF`, three policies) and `vdc_snapshot_mid_dma` (stops fwd, rev, ds, arm), `vdc_stat` checks 16 to 21 (DV: no flag with the enable clear, `$10` alone, cleared by the read; DS: `$20` with DCR bit 0 clear, `$28` with it set, cleared by the read), the
  sweep `mutate_sweep_vdc_satb` (8 judged literals), the ledger pins.

## Verification

- Task verifies, run as written: Task 1 (`--stage fixture-lint`, `--stage conformance`, RESULT greps vdc_dma and vdc_dma_vram), Task 2 (`--stage conformance`, RESULT greps mutate_sweep_vdc_dma, vdc_advance_pure, video_vce_dot, `--stage video-check`), Task 3 (`--stage conformance`, RESULT greps vdc_satb, vdc_satb_data, vdc_snapshot_mid_dma, vdc_stat, `--stage sanitizers`):
  PASS. The tracer verify was re-run on the committed tree as the tracer gate.
- Full `sh scripts/check` PASS before each of the three commits (about 4.5 min each). `sh scripts/check --selftest` PASS (no FAIL). `sh scripts/audit-history --range 71535cf..HEAD` clean (3 commits).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, -Wconversion -Wshadow -Werror, linux/amd64, 56 s). **Wave W4 end:** `LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc' sh scripts/linux-repro` PASS for all three parts (85 s). Both are one-off local runs, not a CI claim. `dash scripts/check` and the second-awk whole-gate runs are
  owed at the Phase 4 checkpoint and close-out.
- **Controls shown able to fail (ER-11), each on the built runner with the source restored byte for byte (cmp):** a divisor of word time + 1 turns vdc_advance_pure red (vdc_dma stays green: 17 against 16 clocks is invisible to a counted delay); a skipped cancel turns vdc_advance_pure and vdc_dma red (the overrun rows); no apply before the events turns vdc_advance_pure red; an
  instant transfer (all words applied at the start) turns exactly check 1 of vdc_dma red (pass=23 fail=1) and vdc_advance_pure red; a SATB word stored changed turns vdc_satb_data red and leaves vdc_satb green by design (the guest sees only DS); a DVSSR low byte that arms turns vdc_satb check 1 red; a DS that ignores DCR bit 0 turns vdc_satb and vdc_stat red (one check each); a
  DV that ignores DCR bit 1 turns vdc_dma and vdc_stat red (one check each); a restore that drops dma_done turns vdc_snapshot_mid_dma red at the first stop. mutate_sweep_vdc_dma and mutate_sweep_vdc_satb flip every judged literal and each flip turns exactly its own check red.
- **Linux gcc and clang** build the whole matrix at -O0 strict C11 with the same results as the local run.
- Evidence class E0 throughout. Rows: GUEST (the additive sum against the assembler multiplication), MAN H7-4 (DV and DS gating and clear on read), H7-11 (the DCR step bits), H7-12 (LENR + 1 words, the high byte trigger, the following-VBlank start), ASSUMED LED-VID-0014 (the copy takes time, IW ignored, the overlap, the registers left alone, the overrun cut, the lone high byte
  retrigger) and ASSUMED LED-VID-0013 (DS follows the copy, DSR repeat); the two poll counts are observe rows judged by no literal. No speed, completion time, throughput, accuracy, compatibility or hardware claim; a pass is not a support claim; no ledger status changed.

## Deviations from Plan

1. **[Rule 3 - Blocking] The core part of all three tasks is in the Task 1 commit.** The plan spreads burst, cancel and the SATB copy over Tasks 2 and 3, but the fold has to hold all of them to be coherent, and the BURST flag changes the vce_pal stream so the golden repin could not wait (Task 1 had to be green on its own). Tasks 2 and 3 changed no file under src/core. The
  repin: old 03fa9142..., new a33b1f0e..., the literal case video_vce_pal passed on the changed records first, recomputed with `shasum -a 256` over the --video-out file and equal to the digest the runner printed.
2. **[Note] Completions are applied in time order inside the fold, not stored as events.** The plan says DV and DS completions are events in the next-event minimum. The same observable order is kept by applying the words due, the DV and the DS time before every ladder event and at the target stamp; the gate of DCR is read at that fold, which is the gate value of the completion
  instant. `ds_vdc_pending_time` and the restore time windows of 04-19 are unchanged.
3. **[Note] `src/core/ds_vdc.h` is edited** (three declarations: `ds_vdc_dma_apply`, `ds_vdc_satb_copy`, `ds_vdc_satb_due`); it is not in the plan's list.
4. **[Rule 3 - Blocking] `tests/conformance/obskinds.c`: the opcode inventory sink went from 262,144 to 1,048,576 records.** The polling guests (vdc_satb, the longer vdc_stat) execute more than a quarter of a million instructions before their DSV1 verdict and opcode_exercise refuses a run that lost a record. The kind still requires lost = 0.
5. **[Rule 1 - Bug] `model_param_vdc` until_mclk 3,000,000 to 2,100,000.** vdc_stat gained sections F and G after its old end (about 2.0 million master clocks); G reads the status with bit 5 set after a counted wait, so with the old deadline that read would become "the last read with bit 5" and the measurement would silently use a different event. At 2,100,000 the measurement
  is the old one (vd_shift 105 against the literal 96, tol 15); a comment in the case says why. No literal or tolerance changed.
6. **[Note] vdc_stat until_mclk 3,000,000 to 5,000,000** (vdc_stat and mutate_sweep_vdc_stat): the two new sections wait for two more VBlanks. The image, the checks file, the manifest pin and both case check counts moved together (15 to 21).
7. **[Note] 24 checks in vdc_dma and 9 in vdc_satb, not about 18 and about 10.** Every row of the behaviour list is covered; the extra rows are the two halves of the SI/D and DI/D rows, the three untouched-register rows and the overrun rows.
8. **[Note] The BURST flag is set on display lines only**, not on every line of a burst frame (see key decisions). The plan text "records of a burst frame carry flag BURST" is read as the display period.
9. **[Note] `vdc_snapshot_mid_dma` uses `fixture = authored:vdc-dma`** (the kind boots both ROMs by name from its expect); the field only has to name a built ROM.
10. **[Note] `got` strings are capped at 63 characters by the runner**; the first version of vdc_advance_pure printed a longer line and the tail (the pending count) was cut. The coverage minimums of the kind (cancels at least scenarios/16, completions scenarios/4, pending starts scenarios/16) are liveness bounds of the test, not behaviour literals.
11. **[Note] The retrigger row is a LENR low byte write alone (stores, no start) followed by a lone high byte write with the stored low byte** (check 19 MAN H7-12, check 20 ASSUMED LED-VID-0014); the plan's "retrigger with only the LENR MSB" needs a stored low byte to be a defined transfer length.
12. **[Note] Plan estimate was 120,000 tokens; the realized diff is about 26,300** by chars/4 over the added lines (37 files, 1,523 insertions, 40 deletions). The stop rule did not fire.
13. **[Note] The standard plan-complete step may tick VID-03 or VID-04** in REQUIREMENTS.md (see the close-out note).

## Known Stubs

For a display line of a frame that is not burst the payload is still the blank index 256 (D-157 gives index 0 where BB = 0 and the background where BB = 1); 04-11 replaces it. Not a new stub of this plan: it was the interim state of 04-09, and only the BURST flag of burst frames is new here.

## Threat Flags

None. T-04-34 (guest addresses and lengths) is mitigated: every word goes through the bounds-checked accessor pair, the step arithmetic is unsigned modulo 65,536, at most 65,536 words per transfer, and the sanitizers stage runs vdc_dma, vdc_advance_pure (out-of-range source and destination words included) and vdc_snapshot_mid_dma. T-04-35 (the progress loop) by the closed form
  (one loop over the due words, capped by LENR + 1) and by the zero-divisor guards. T-04-36 (SATB copy from a hostile DVSSR) by 256 fixed words through the accessor with (DVSSR + i) & $FFFF; SATB is written only by the copy. T-04-37 (claim inflation): the duration rows are observe rows, the expected-red control is ASSUMED, the ledger withholds any speed claim.

## Notes for next plans

- **The DMA fields are live in the fold.** `ds_vdc_dma_apply` and `ds_vdc_satb_due` run before every ladder event and at the end of `ds_vdc_advance`; a plan that adds another time-dependent VDC source (04-15 OR and CR flags, 04-13 sprite flags) adds its calls there. The burst latch is `inst->burst` set in `dma_line`; 04-11 uses it for the backdrop and must keep the blank
  payload identical for the burst frames or repin `video_vce_pal_golden` in the same commit (independent `shasum -a 256` over the --video-out file; `mkdir build/gate/video` first, remove it after). Current digest a33b1f0eb45e7b32c3aab68d5b08023a7c11f7c13cb77ebe4a4bb677b4802364.
- **vdma.inc is final** (macros and equates only; 04-14 may use it unchanged): VDMA_GO, VDMA_PEEK, VDMA_POKE, VDMA_READ_SUM, VDMA_DELAY, VDMA_WAIT_VD and the VR_* and DCR_* equates. DSV ids 20 (vdc_dma) and 21 (vdc_satb) are taken; the next free id is 22.
- **Editing vdc_dma, vdc_satb or vdc_stat later** moves the image: the manifest sha, the checks file, the `checks` and `until_mclk` of the cases, the sweep exclusions and `vdc_dma_vram` words. vdc_stat now reads the status with bit 5 set in section G; `model_param_vdc` depends on its deadline staying before that read.
- **vdc_stat DS and DV rows are done** (checks 16 to 21); the OR and CR rows join in 04-15. The opcode inventory sink is 1,048,576 records; a later polling guest should check its instruction count against it.
- **Mid-transfer restores** are judged by `vdc_snapshot_mid_dma`; the four stop kinds (fwd, rev, ds, arm) are in its expect and a new DMA ROM adds an entry there.
- **Owed:** `dash scripts/check` and the second-awk whole-gate runs at the Phase 4 checkpoint and close-out. The conformance stage is about 57 s locally now (trigger 60 s); D-176 levers apply before it grows further. The vdc_satb and vdc_stat guests run about 5 million master clocks each under three policies.
- `.planning/state.json` was left untouched and uncommitted.

## Close-out note on requirement ticks

The standard plan-complete step (`requirements.mark-complete VID-03`) ticked the VID-03 checkbox and its traceability row. `requirements.ready-ids` called with the phase and the id returned VID-03 ready, but it returned every id ready (VID-01, VID-05 and VID-06 too), including ones that 04-13 and 04-16 still list without a SUMMARY, so its answer was not taken as the rule: the
plan's late tick rule holds an id while another plan of the phase that lists it lacks a SUMMARY (04-13 and 04-16 list VID-03). The tick was reverted with a file-specific `git checkout` of .planning/REQUIREMENTS.md (nothing else in that file was modified), so REQUIREMENTS.md equals HEAD and the normal step of 04-16 ticks it. The orchestrator may re-run
`requirements.mark-complete VID-03` if it decides otherwise; the call is idempotent.

## Self-Check: PASSED

- FOUND: fixtures/lib/vdma.inc, fixtures/src/vdc_dma.s, fixtures/src/vdc_satb.s, both manifests, the eight new case files, REQUIRED lines for the eight new ids.
- FOUND commits bbba639, c8f5c2c and f89f6d4 (ancestors of HEAD).
